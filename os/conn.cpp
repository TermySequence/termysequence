// Copyright © 2018 TermySequence LLC
//
// SPDX-License-Identifier: GPL-2.0-only

#include "common.h"
#include "conn.h"
#include "lib/exception.h"

#include <sys/un.h>
#include <unistd.h>
#include <fcntl.h>
#include <poll.h>

#ifdef __linux__
#   define SD_NONBLOCK(fd)
#   define SD_CLOEXEC(fd)
#   define SD_ACCEPT(fd, a, b) accept4(fd, a, b, SOCK_CLOEXEC|SOCK_NONBLOCK)
#   define SD_PEERCRED_FAM SOL_SOCKET
#   define SD_PEERCRED_OPT SO_PEERCRED
#   define SD_PEERCRED_UID(ucred) (ucred.uid)
#   define SD_PEERCRED_PID(ucred) (ucred.pid)
#   define SD_PIPE(fd) pipe2(fd, O_CLOEXEC|O_NONBLOCK)
typedef struct ucred sd_ucred;
#else /* Mac OS X */
#   define SOCK_NONBLOCK 0
#   define SOCK_CLOEXEC 0
#   define MSG_CMSG_CLOEXEC 0
#   define SD_NONBLOCK(fd) osMakeNonblocking(fd)
#   define SD_CLOEXEC(fd) osMakeCloexec(fd)
#   define SD_ACCEPT(fd, a, b) accept(fd, a, b)
#   include <sys/ucred.h>
#   define SD_PEERCRED_FAM SOL_LOCAL
#   define SD_PEERCRED_OPT LOCAL_PEERCRED
#   define SD_PEERCRED_UID(ucred) (ucred.cr_uid)
#   define SD_PEERCRED_PID(ucred) (-1)
#   define SD_PIPE(fd) pipe(fd)
typedef struct xucred sd_ucred;
#endif

void
osMakeNonblocking(int fd)
{
    // Set nonblocking
    int flags = fcntl(fd, F_GETFL, 0);
    fcntl(fd, F_SETFL, flags | O_NONBLOCK);
}

void
osMakeBlocking(int fd)
{
    // Set blocking
    int flags = fcntl(fd, F_GETFL, 0);
    fcntl(fd, F_SETFL, flags & ~O_NONBLOCK);
}

void
osMakeCloexec(int fd)
{
    // Set cloexec
    int flags = fcntl(fd, F_GETFD, 0);
    fcntl(fd, F_SETFD, flags | O_CLOEXEC);
}

int
osLocalConnect(const char *path)
{
    int rc, sd;
    struct sockaddr_un sa;

    memset(&sa, 0, sizeof(sa));
    sa.sun_family = AF_UNIX;
    strncpy(sa.sun_path, path, sizeof(sa.sun_path) - 1);

    sd = socket(AF_UNIX, SOCK_STREAM|SOCK_CLOEXEC, 0);
    SD_CLOEXEC(sd);
    if (sd == -1)
        throw Tsq::ErrnoException("socket", errno);
    if (connect(sd, (struct sockaddr *)&sa, sizeof(sa)) == 0) {
        osMakeNonblocking(sd);
        return sd;
    }

    rc = errno;
    close(sd);

    // check for unexpected errors
    if (rc != ENOENT && rc != ECONNREFUSED)
        throw Tsq::ErrnoException("connect", path, rc);

    return -1;
}

int
osLocalListen(const char *path, int backlog)
{
    int rc, sd;
    struct sockaddr_un sa;

    memset(&sa, 0, sizeof(sa));
    sa.sun_family = AF_UNIX;
    strncpy(sa.sun_path, path, sizeof(sa.sun_path) - 1);

    sd = socket(AF_UNIX, SOCK_STREAM|SOCK_NONBLOCK|SOCK_CLOEXEC, 0);
    SD_CLOEXEC(sd);
    SD_NONBLOCK(sd);
    if (sd < 0) {
        throw Tsq::ErrnoException("socket", errno);
    }

    rc = bind(sd, (struct sockaddr *)&sa, sizeof(sa));
    if (rc != 0) {
        throw Tsq::ErrnoException("bind", path, errno);
    }

    rc = listen(sd, backlog);
    if (rc != 0) {
        throw Tsq::ErrnoException("listen", path, errno);
    }

    return sd;
}

void
osLocalReceiveFd(int fd, int *payload, int n)
{
    struct msghdr msg = { 0 };
    struct iovec iov = { 0 };
    union {
        char buf[CMSG_SPACE(2 * sizeof(int))];
        struct cmsghdr align;
    } control;
    struct cmsghdr *cmsg;
    ssize_t rc;
    char c;

    iov.iov_base = &c;
    iov.iov_len = 1;

    msg.msg_iov = &iov;
    msg.msg_iovlen = 1;
    msg.msg_control = control.buf;
    msg.msg_controllen = CMSG_SPACE(n * sizeof(int));

    rc = recvmsg(fd, &msg, MSG_CMSG_CLOEXEC);
    if (rc <= 0)
        throw Tsq::ErrnoException("recvmsg", rc ? errno : EIO);

    cmsg = CMSG_FIRSTHDR(&msg);
    if (!cmsg || cmsg->cmsg_level != SOL_SOCKET || cmsg->cmsg_type != SCM_RIGHTS)
        throw Tsq::ErrnoException("recvmsg", EINVAL);
    if (cmsg->cmsg_len != CMSG_LEN(n * sizeof(int)))
        throw Tsq::ErrnoException("recvmsg", EINVAL);

    memcpy(payload, CMSG_DATA(cmsg), n * sizeof(int));
    while (n--) {
        SD_CLOEXEC(payload[n]);
        osMakeNonblocking(payload[n]);
    }
}

int
osLocalSendFd(int fd, const int *payload, int n)
{
    struct msghdr msg = { 0 };
    struct iovec iov = { 0 };
    union {
        char buf[CMSG_SPACE(2 * sizeof(int))];
        struct cmsghdr align;
    } control;
    struct cmsghdr *cmsg;
    char c = '\0';

    iov.iov_base = &c;
    iov.iov_len = 1;

    msg.msg_iov = &iov;
    msg.msg_iovlen = 1;
    msg.msg_control = control.buf;
    msg.msg_controllen = CMSG_SPACE(n * sizeof(int));

    cmsg = CMSG_FIRSTHDR(&msg);
    cmsg->cmsg_level = SOL_SOCKET;
    cmsg->cmsg_type = SCM_RIGHTS;
    cmsg->cmsg_len = CMSG_LEN(n * sizeof(int));
    memcpy(CMSG_DATA(cmsg), payload, n * sizeof(int));

    return sendmsg(fd, &msg, 0);
}

int
osLocalCredsCheck(int fd)
{
    sd_ucred cred = { 0 };
    socklen_t len = sizeof(sd_ucred);
    uid_t uid = getuid();

    if (getsockopt(fd, SD_PEERCRED_FAM, SD_PEERCRED_OPT, &cred, &len) < 0)
        throw Tsq::ErrnoException("getsockopt", errno);
    if (SD_PEERCRED_UID(cred) != uid)
        throw Tsq::ErrnoException(EPERM);

    return SD_PEERCRED_PID(cred);
}

bool
osLocalCreds(int fd, int &uidret, int &pidret)
{
    sd_ucred cred = { 0 };
    socklen_t len = sizeof(sd_ucred);

    if (getsockopt(fd, SD_PEERCRED_FAM, SD_PEERCRED_OPT, &cred, &len) == 0) {
        uidret = SD_PEERCRED_UID(cred);
        pidret = SD_PEERCRED_PID(cred);
        return true;
    } else {
        uidret = pidret = -1;
        return false;
    }
}

int
osAccept(int listensd, struct sockaddr *buf, socklen_t *buflen)
{
    int sd = SD_ACCEPT(listensd, buf, buflen);
    SD_CLOEXEC(sd);
    SD_NONBLOCK(sd);
    if (sd < 0) {
        if (errno == EAGAIN || errno == EWOULDBLOCK)
            return -1;
        if (errno == EINTR)
            return -1;

        throw Tsq::ErrnoException("accept", errno);
    }

    return sd;
}

void
osWaitForWritable(int fd)
{
    pollfd pfd;
    pfd.fd = fd;
    pfd.events = POLLOUT;

    poll(&pfd, 1, 1000);
}

void
osSocketPair(int sd[2], bool bothCloexec)
{
    if (socketpair(AF_UNIX, SOCK_STREAM|SOCK_NONBLOCK|SOCK_CLOEXEC, 0, sd))
        throw Tsq::ErrnoException("socketpair", errno);

    SD_CLOEXEC(sd[0]);
    SD_CLOEXEC(sd[1]);
    SD_NONBLOCK(sd[0]);
    SD_NONBLOCK(sd[1]);

    if (!bothCloexec) {
        // Remove cloexec
        int flags = fcntl(sd[1], F_GETFD, 0);
        fcntl(sd[1], F_SETFD, flags & ~O_CLOEXEC);
    }
}

int
osSocket(int domain, int type, int protocol)
{
    int sd = socket(domain, type|SOCK_NONBLOCK|SOCK_CLOEXEC, protocol);
    SD_CLOEXEC(sd);
    SD_NONBLOCK(sd);
    return sd;
}

int
osPipe(int fd[2])
{
    int rc = SD_PIPE(fd);
    if (rc == 0) {
        SD_CLOEXEC(fd[0]);
        SD_CLOEXEC(fd[1]);
        SD_NONBLOCK(fd[0]);
        SD_NONBLOCK(fd[1]);
    }
    return rc;
}
