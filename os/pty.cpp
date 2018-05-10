// Copyright © 2018 TermySequence LLC
//
// SPDX-License-Identifier: GPL-2.0-only

#include "common.h"
#include "pty.h"
#include "time.h"
#include "lib/flags.h"
#include "lib/exception.h"
#include "lib/base64.h"
#include "lib/sequences.h"
#include "config.h"

#include <sys/types.h>
#include <sys/ioctl.h>
#include <unistd.h>
#include <fcntl.h>
#include <poll.h>
#include <termios.h>

#ifndef __linux__
#define IUCLC 0
#define OLCUC 0
#define XTABS OXTABS
#endif

void
osSetTerminal(int fd)
{
    ioctl(fd, TIOCSCTTY, NULL);
}

void
osResizeTerminal(int fd, unsigned short width, unsigned short height)
{
    // do not throw from here
    struct winsize winsize = { height, width, 0, 0 };

    ioctl(fd, TIOCSWINSZ, &winsize);
}

bool
osGetTerminalLockable(int fd)
{
    struct termios buf;

    return (tcgetattr(fd, &buf) == 0) &&
        (buf.c_iflag & IXON) &&
        (buf.c_cc[VSTOP] == 0x13) &&
        (buf.c_cc[VSTART] == 0x11);
}

void
osMakeRawTerminal(int fd, char **savedret)
{
    struct termios buf{};
    tcgetattr(fd, &buf);

    if (savedret) {
        *savedret = new char[sizeof(buf)];
        memcpy(*savedret, &buf, sizeof(buf));
    }

    cfmakeraw(&buf);
    buf.c_cc[VMIN] = 1;
    buf.c_cc[VTIME] = 0;

    tcsetattr(fd, TCSAFLUSH, &buf);
}

void
osMakeSignalTerminal(int fd, char **savedret)
{
    struct termios buf{};
    tcgetattr(fd, &buf);

    *savedret = new char[sizeof(buf)];
    memcpy(*savedret, &buf, sizeof(buf));

    cfmakeraw(&buf);
    buf.c_lflag |= TOSTOP;
    buf.c_cc[VMIN] = 1;
    buf.c_cc[VTIME] = 0;

    tcsetattr(fd, TCSAFLUSH, &buf);
}

void
osMakePasswordTerminal(int fd, char **savedret)
{
    struct termios buf;

    if (tcgetattr(fd, &buf) < 0)
        throw Tsq::ErrnoException("tcgetattr", errno);

    *savedret = new char[sizeof(buf)];
    memcpy(*savedret, &buf, sizeof(buf));

    buf.c_lflag &= ~ECHO;

    if (tcsetattr(fd, TCSAFLUSH, &buf) < 0)
        throw Tsq::ErrnoException("tcsetattr", errno);
}

void
osRestoreTerminal(int fd, char *saved)
{
    tcsetattr(fd, TCSAFLUSH, (struct termios*)saved);
}

static bool
writeEmulatorQuery(const std::string &value)
{
    int64_t timelimit = osMonotime() + OSC_QUERY_TIMEOUT;
    size_t left = value.size();
    const char *ptr = value.data();

    while (left) {
        ssize_t rc = write(STDIN_FILENO, ptr, left);
        if (rc >= 0) {
            left -= rc;
            ptr += rc;
        }
        else if (errno == EAGAIN) {
            pollfd pfd = { .fd = STDIN_FILENO, .events = POLLOUT };
            int64_t timeout = timelimit - osMonotime();

            if (timeout <= 0 || poll(&pfd, 1, timeout) <= 0)
                return false;
        }
        else {
            return false;
        }
    }
    return true;
}

bool
osGetEmulatorAttribute(const char *name, std::string &value, bool *found)
{
    value.assign(TSQ_GETVARPREFIX, TSQ_GETVARPREFIX_LEN);
    value.append(name);
    value.push_back('\a');

    // Write the query
    if (!writeEmulatorQuery(value))
        return false;

    // Read the reply
    int64_t timelimit = osMonotime() + OSC_QUERY_TIMEOUT;
    pollfd pfd = { .fd = STDIN_FILENO, .events = POLLIN };
    char buf[4096] = {}, *ptr, *ptr2;
    size_t got = 0;

    while (1) {
        ssize_t rc = read(pfd.fd, buf + got, sizeof(buf) - 1 - got);
        if (rc > 0) {
            if ((ptr = strchr(buf, '\a'))) {
                *ptr = '\0';
                break;
            }
            if ((got += rc) == sizeof(buf) - 1)
                return false;
        }
        else if (rc < 0 && errno == EAGAIN) {
            int64_t timeout = timelimit - osMonotime();
            if (timeout <= 0 || poll(&pfd, 1, timeout) <= 0)
                return false;
        }
        else {
            return false;
        }
    }

    // Parse the reply
    ptr = strchr(buf, ';');
    if (!ptr)
        return false;
    ptr2 = strchr(++ptr, '=');
    if (!ptr2) {
        if (found)
            *found = false;
        return !strcmp(ptr, name);
    }
    *ptr2++ = '\0';
    if (strcmp(ptr, name))
        return false;
    if (found)
        *found = true;
    value.assign(ptr2);
    return unbase64_inplace_utf8(value);
}

bool
osClearEmulatorAttribute(const char *name)
{
    std::string data(TSQ_SETVARPREFIX, TSQ_SETVARPREFIX_LEN);
    data.append(name);
    data.push_back('\a');

    // Write the query
    return writeEmulatorQuery(data);
}

bool
osSetEmulatorAttribute(const char *name, const std::string &value)
{
    std::string data;
    base64(value.data(), value.size(), data);
    data.insert(data.begin(), '=');
    data.insert(0, name);
    data.insert(0, TSQ_SETVARPREFIX, TSQ_SETVARPREFIX_LEN);
    data.push_back('\a');

    // Write the query
    return writeEmulatorQuery(data);
}

bool
osGetTerminalAttributes(int fd, uint32_t *termiosFlags, char *termiosChars)
{
    struct termios buf;

    if (tcgetattr(fd, &buf) == 0) {
        Tsq::TermiosFlags iflags, oflags, lflags;

        iflags = (buf.c_iflag & IGNBRK)  ? Tsq::TermiosIGNBRK : 0;
        iflags |= (buf.c_iflag & BRKINT)  ? Tsq::TermiosBRKINT : 0;
        iflags |= (buf.c_iflag & IGNPAR)  ? Tsq::TermiosIGNPAR : 0;
        iflags |= (buf.c_iflag & PARMRK)  ? Tsq::TermiosPARMRK : 0;
        iflags |= (buf.c_iflag & INPCK)   ? Tsq::TermiosINPCK  : 0;
        iflags |= (buf.c_iflag & ISTRIP)  ? Tsq::TermiosISTRIP : 0;
        iflags |= (buf.c_iflag & INLCR)   ? Tsq::TermiosINLCR  : 0;
        iflags |= (buf.c_iflag & IGNCR)   ? Tsq::TermiosIGNCR  : 0;
        iflags |= (buf.c_iflag & ICRNL)   ? Tsq::TermiosICRNL  : 0;
        iflags |= (buf.c_iflag & IUCLC)   ? Tsq::TermiosIUCLC  : 0;
        iflags |= (buf.c_iflag & IXON)    ? Tsq::TermiosIXON   : 0;
        iflags |= (buf.c_iflag & IXANY)   ? Tsq::TermiosIXANY  : 0;
        iflags |= (buf.c_iflag & IXOFF)   ? Tsq::TermiosIXOFF  : 0;

        oflags = (buf.c_oflag & OPOST)   ? Tsq::TermiosOPOST  : 0;
        oflags |= (buf.c_oflag & OLCUC)   ? Tsq::TermiosOLCUC  : 0;
        oflags |= (buf.c_oflag & ONLCR)   ? Tsq::TermiosONLCR  : 0;
        oflags |= (buf.c_oflag & OCRNL)   ? Tsq::TermiosOCRNL  : 0;
        oflags |= (buf.c_oflag & ONOCR)   ? Tsq::TermiosONOCR  : 0;
        oflags |= (buf.c_oflag & ONLRET)  ? Tsq::TermiosONLRET : 0;
        oflags |= ((buf.c_oflag & XTABS) == XTABS) ? Tsq::TermiosOXTABS : 0;

        lflags = (buf.c_lflag & ISIG)    ? Tsq::TermiosISIG   : 0;
        lflags |= (buf.c_lflag & ICANON)  ? Tsq::TermiosICANON : 0;
        lflags |= (buf.c_lflag & ECHO)    ? Tsq::TermiosECHO   : 0;
        lflags |= (buf.c_lflag & ECHOE)   ? Tsq::TermiosECHOE  : 0;
        lflags |= (buf.c_lflag & ECHOK)   ? Tsq::TermiosECHOK  : 0;
        lflags |= (buf.c_lflag & ECHONL)  ? Tsq::TermiosECHONL : 0;
        lflags |= (buf.c_lflag & NOFLSH)  ? Tsq::TermiosNOFLSH : 0;
        lflags |= (buf.c_lflag & TOSTOP)  ? Tsq::TermiosTOSTOP : 0;
        lflags |= (buf.c_lflag & ECHOCTL) ? Tsq::TermiosECHOCTL : 0;
        lflags |= (buf.c_lflag & ECHOPRT) ? Tsq::TermiosECHOPRT : 0;
        lflags |= (buf.c_lflag & ECHOKE)  ? Tsq::TermiosECHOKE : 0;
        lflags |= (buf.c_lflag & FLUSHO)  ? Tsq::TermiosFLUSHO : 0;
        lflags |= (buf.c_lflag & PENDIN)  ? Tsq::TermiosPENDIN : 0;
        lflags |= (buf.c_lflag & IEXTEN)  ? Tsq::TermiosIEXTEN : 0;

        termiosChars[Tsq::TermiosVEOF]     = buf.c_cc[VEOF];
        termiosChars[Tsq::TermiosVEOL]     = buf.c_cc[VEOL];
        termiosChars[Tsq::TermiosVEOL2]    = buf.c_cc[VEOL2];
        termiosChars[Tsq::TermiosVERASE]   = buf.c_cc[VERASE];
        termiosChars[Tsq::TermiosVWERASE]  = buf.c_cc[VWERASE];
        termiosChars[Tsq::TermiosVKILL]    = buf.c_cc[VKILL];
        termiosChars[Tsq::TermiosVREPRINT] = buf.c_cc[VREPRINT];
        termiosChars[Tsq::TermiosVINTR]    = buf.c_cc[VINTR];
        termiosChars[Tsq::TermiosVQUIT]    = buf.c_cc[VQUIT];
        termiosChars[Tsq::TermiosVSUSP]    = buf.c_cc[VSUSP];
#ifdef VDSUSP
        termiosChars[Tsq::TermiosVDSUSP]   = buf.c_cc[VDSUSP];
#endif
        termiosChars[Tsq::TermiosVSTART]   = buf.c_cc[VSTART];
        termiosChars[Tsq::TermiosVSTOP]    = buf.c_cc[VSTOP];
        termiosChars[Tsq::TermiosVLNEXT]   = buf.c_cc[VLNEXT];
        termiosChars[Tsq::TermiosVDISCARD] = buf.c_cc[VDISCARD];
        termiosChars[Tsq::TermiosVMIN]     = buf.c_cc[VMIN];
        termiosChars[Tsq::TermiosVTIME]    = buf.c_cc[VTIME];
#ifdef VSTATUS
        termiosChars[Tsq::TermiosVSTATUS]  = buf.c_cc[VSTATUS];
#endif

        termiosFlags[0] = iflags;
        termiosFlags[1] = oflags;
        termiosFlags[2] = lflags;
        return true;
    }
    return false;
}
