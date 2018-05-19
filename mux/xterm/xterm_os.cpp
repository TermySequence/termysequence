// Copyright © 2018 TermySequence LLC
//
// SPDX-License-Identifier: GPL-2.0-only

#include "common.h"
#include "xterm.h"
#include "base16.h"
#include "base/term.h"
#include "base/palette.h"
#include "lib/attr.h"
#include "lib/attrstr.h"
#include "lib/base64.h"
#include "config.h"

#include <cstdio>

using namespace std;

void
XTermEmulator::cmdModeRequest()
{
    int mode = m_state.var(0).toUInt(), reply = -1;

    switch (mode) {
    case 2:
        reply = !(m_flags & Tsq::KeyboardLock);
        break;
    case 4:
        reply = !(m_flags & Tsq::InsertMode);
        break;
    case 12:
        reply = !(m_flags & Tsq::SendReceive);
        break;
    case 20:
        reply = !(m_flags & Tsq::NewLine);
        break;
    }

    char buf[32];
    snprintf(buf, sizeof(buf), "\xc2\x9b%d;%d$y", mode, reply + 1);
    termReply(buf);
}

void
XTermEmulator::cmdDECPrivateModeRequest()
{
    int mode = m_state.var(0).toUInt(), reply = -1;

    switch (mode) {
    case 1:
        reply = !(m_flags & Tsq::AppCuKeys);
        break;
    case 2:
        reply = !(m_flags & Tsq::Ansi);
        break;
    case 3:
        reply = m_screen->width() != 132;
        break;
    case 4:
        reply = !(m_flags & Tsq::SmoothScrolling);
        break;
    case 5:
        reply = !(m_flags & Tsq::ReverseVideo);
        break;
    case 6:
        reply = !(m_flags & Tsq::OriginMode);
        break;
    case 7:
        reply = !(m_flags & Tsq::Autowrap);
        break;
    case 8:
        reply = !(m_flags & Tsq::Autorepeat);
        break;
    case 9:
        reply = !(m_flags & Tsq::X10MouseMode);
        break;
    case 12:
        reply = !(m_cursorStyle & 1);
        break;
    case 25:
        reply = !(m_flags & Tsq::CursorVisible);
        break;
    case 40:
        reply = !(m_flags & Tsq::AllowColumnChange);
        break;
    case 45:
        reply = !(m_flags & Tsq::ReverseAutowrap);
        break;
    case 47:
        reply = !(m_flags & Tsq::AppScreen);
        break;
    case 66:
        reply = !(m_flags & Tsq::AppKeyPad);
        break;
    case 69:
        reply = !(m_flags & Tsq::LeftRightMarginMode);
        break;
    case 1000:
        reply = !(m_flags & Tsq::NormalMouseMode);
        break;
    case 1001:
        reply = !(m_flags & Tsq::HighlightMouseMode);
        break;
    case 1002:
        reply = !(m_flags & Tsq::ButtonEventMouseMode);
        break;
    case 1003:
        reply = !(m_flags & Tsq::AnyEventMouseMode);
        break;
    case 1004:
        reply = !(m_flags & Tsq::FocusEventMode);
        break;
    case 1005:
        reply = !(m_flags & Tsq::Utf8ExtMouseMode);
        break;
    case 1006:
        reply = !(m_flags & Tsq::SgrExtMouseMode);
        break;
    case 1007:
        reply = !(m_flags & Tsq::AltScrollMouseMode);
        break;
    case 1015:
        reply = !(m_flags & Tsq::UrxvtExtMouseMode);
        break;
    case 1047:
    case 1049:
        reply = !(m_flags & Tsq::AppScreen);
        break;
    case 2004:
        reply = !(m_flags & Tsq::BracketedPasteMode);
        break;
    case 1048:
    case 1010:
    case 1011:
    case 1034:
    case 1035:
    case 1036:
    case 1037:
    case 1040:
        // Always set
        reply = 0;
        break;
    case 18:
    case 19:
    case 30:
    case 35:
    case 38:
    case 42:
    case 44:
    case 67:
    case 95:
    case 1039:
    case 1041:
    case 1042:
    case 1043:
        // Always unset
        reply = 1;
        break;
    case 41:
    case 1050:
    case 1051:
    case 1060:
    case 1061:
        // Permanently unset
        reply = 3;
        break;
    }

    char buf[32];
    snprintf(buf, sizeof(buf), "\xc2\x9b?%d;%d$y", mode, reply + 1);
    termReply(buf);
}

void
XTermEmulator::cmdSendDeviceAttributes()
{
    switch (m_state.var(0).toUInt()) {
    case 0:
        termReply("\xc2\x9b?64;1;2;6;9;15;18;21;22c");
        break;
    }
}

void
XTermEmulator::cmdSendDeviceAttributes2()
{
    switch (m_state.var(0).toUInt()) {
    case 0:
        termReply("\xc2\x9b>41;327;0c");
        break;
    }
}

void
XTermEmulator::cmdDeviceStatusReport()
{
    char buf[32];
    Point cursor = m_screen->cursor();

    switch (m_state.var(0).toUInt()) {
    case 5:
        termReply("\xc2\x9b""0n");
        break;
    case 6:
        snprintf(buf, sizeof(buf), "\xc2\x9b%d;%dR", cursor.y() + 1, cursor.x() + 1);
        termReply(buf);
        break;
    }
}

void
XTermEmulator::cmdIgnored()
{
    // What goes here?
    // cmdSelectEncoding()
    // cmdSetConformanceLevel
    // dcsRequestTerminfo + dcsSetTerminfo
    // dcsUserDefinedKeys
    // apc + pm
    // highlight mouse tracking
}

void
XTermEmulator::dcsRequestStatusString()
{
    const Codestring &str = m_state.var(0);
    char buf[256] = "\x18";

    if (str == "\"p") {
        // cmdSetConformanceLevel
        snprintf(buf, sizeof(buf), "\xc2\x90""1$r64;%d\"p\xc2\x9c",
                 !(m_flags & Tsq::Controls8Bit));
    } else if (str == "\"q") {
        // cmdProtectionAttribute
        snprintf(buf, sizeof(buf), "\xc2\x90""1$r%d\"q\xc2\x9c",
                 !!(m_attributes.flags & Tsq::Protected));
    } else if (str == "r") {
        // cmdSetTopBottomMargins
        snprintf(buf, sizeof(buf), "\xc2\x90""1$r%d;%dr\xc2\x9c",
                 m_screen->margins().top() + 1,
                 m_screen->margins().bottom() + 1);
    } else if (str == " q") {
        // cmdSetCursorStyle
        snprintf(buf, sizeof(buf), "\xc2\x90""1$r%u q\xc2\x9c",
                 m_cursorStyle);
    } else if (str == "m") {
        // cmdCharacterAttributes
        strcpy(buf, "\xc2\x90""1$r");
        if (m_attributes.flags & Tsq::Bold)
            strcat(buf, "1;");
        if (m_attributes.flags & Tsq::Faint)
            strcat(buf, "2;");
        if (m_attributes.flags & Tsq::Italics)
            strcat(buf, "3;");
        if (m_attributes.flags & Tsq::Underline)
            strcat(buf, "4;");
        if (m_attributes.flags & Tsq::Blink)
            strcat(buf, "5;");
        if (m_attributes.flags & Tsq::FastBlink)
            strcat(buf, "6;");
        if (m_attributes.flags & Tsq::Inverse)
            strcat(buf, "7;");
        if (m_attributes.flags & Tsq::Invisible)
            strcat(buf, "8;");
        if (m_attributes.flags & Tsq::Strikethrough)
            strcat(buf, "9;");
        if (m_attributes.flags & Tsq::AltFont1)
            strcat(buf, "11;");
        if (m_attributes.flags & Tsq::AltFont2)
            strcat(buf, "12;");
        if (m_attributes.flags & Tsq::AltFont0)
            strcat(buf, "20;");
        if (m_attributes.flags & Tsq::DblUnderline)
            strcat(buf, "21;");
        if (m_attributes.flags & Tsq::Framed)
            strcat(buf, "51;");
        if (m_attributes.flags & Tsq::Encircled)
            strcat(buf, "52;");
        if (m_attributes.flags & Tsq::Overline)
            strcat(buf, "53;");
        if (m_attributes.flags & Tsq::FgIndex) {
            if (m_attributes.fg < 8)
                sprintf(buf + strlen(buf), "%u;", m_attributes.fg + 30);
            else if (m_attributes.fg < 16)
                sprintf(buf + strlen(buf), "%u;", m_attributes.fg + 82);
            else
                sprintf(buf + strlen(buf), "38;5;%u;", m_attributes.fg);
        } else if (m_attributes.flags & Tsq::Fg) {
            sprintf(buf + strlen(buf), "38;2;%u;%u;%u;", COLOR_RED(m_attributes.fg),
                    COLOR_BLUE(m_attributes.fg), COLOR_GREEN(m_attributes.fg));
        }
        if (m_attributes.flags & Tsq::BgIndex) {
            if (m_attributes.bg < 8)
                sprintf(buf + strlen(buf), "%u;", m_attributes.bg + 40);
            else if (m_attributes.bg < 16)
                sprintf(buf + strlen(buf), "%u;", m_attributes.bg + 92);
            else
                sprintf(buf + strlen(buf), "48;5;%u;", m_attributes.bg);
        } else if (m_attributes.flags & Tsq::Bg) {
            sprintf(buf + strlen(buf), "48;2;%u;%u;%u;", COLOR_RED(m_attributes.bg),
                    COLOR_BLUE(m_attributes.bg), COLOR_GREEN(m_attributes.bg));
        }
        if (!buf[5])
            strcat(buf, "0m\xc2\x9c");
        else
            strcpy(buf + strlen(buf) - 1, "m\xc2\x9c");
    }

    termReply(buf);
}

static void
rejectControlCodes(string &str)
{
    for (Codepoint c: Codestring(str))
        if (XTermStateMachine::isControlCode(c)) {
            str.clear();
            break;
        }
}

void
XTermEmulator::cmdWindowOps()
{
    string data;
    char buf[32];
    int arg = m_state.var(0).toUInt();

    switch (arg) {
    case 11:
        termReply("\xc2\x9b""1t");
        break;
    case 13:
        termReply("\xc2\x9b""3;0;0t");
        break;
    case 14:
        termReply("\xc2\x9b""4;600;800t");
        break;
    case 18:
    case 19:
        snprintf(buf, sizeof(buf), "\xc2\x9b%d;%d;%dt", arg - 10,
                 m_screen->height(), m_screen->width());
        termReply(buf);
        break;
    case 20:
        data = m_parent->getAttribute(Tsq::attr_SESSION_TITLE2);
        rejectControlCodes(data);
        if (m_flags & Tsq::TitleModeQueryHex)
            data = base16(data);
        data.insert(0, "\xc2\x9dL");
        data.append("\xc2\x9c");
        termReply(data.data());
        break;
    case 21:
        data = m_parent->getAttribute(Tsq::attr_SESSION_TITLE);
        rejectControlCodes(data);
        if (m_flags & Tsq::TitleModeQueryHex)
            data = base16(data);
        data.insert(0, "\xc2\x9dl");
        data.append("\xc2\x9c");
        termReply(data.data());
        break;
    case 22:
        switch (arg = m_state.var(1).toUInt()) {
        case 0:
            m_titleStack.push(m_parent->getAttribute(Tsq::attr_SESSION_TITLE));
            // fallthru
        case 1:
            m_title2Stack.push(m_parent->getAttribute(Tsq::attr_SESSION_TITLE2));
            break;
        case 2:
            m_titleStack.push(m_parent->getAttribute(Tsq::attr_SESSION_TITLE));
        }
        break;
    case 23:
        switch (arg = m_state.var(1).toUInt()) {
        case 0:
        case 1:
            if (!m_title2Stack.empty()) {
                setAttribute(Tsq::attr_SESSION_TITLE2, m_title2Stack.top());
                m_title2Stack.pop();
            }
            if (arg == 1)
                break;
            // fallthru
        case 2:
            if (!m_titleStack.empty()) {
                setAttribute(Tsq::attr_SESSION_TITLE, m_titleStack.top());
                m_titleStack.pop();
            }
        }
        break;
    }
}

void
XTermEmulator::osc0(string &str, int arg)
{
    if (m_flags & Tsq::TitleModeSetHex && !unbase16_inplace_utf8(str))
        return;
    rejectControlCodes(str);

    switch (arg) {
    case 0:
        setAttribute(Tsq::attr_SESSION_TITLE, str);
        // fallthru
    case 1:
        setAttribute(Tsq::attr_SESSION_TITLE2, str);
        break;
    case 2:
        setAttribute(Tsq::attr_SESSION_TITLE, str);
        break;
    }
}

void
XTermEmulator::osc3(string str)
{
    str.insert(0, Tsq::attr_PROP_PREFIX);
    size_t idx = str.find('=');

    if (idx != string::npos) {
        setAttribute(str.substr(0, idx), str.substr(idx + 1));
    } else {
        removeAttribute(str);
    }
}

static bool
colorParse(string &spec, unsigned *result)
{
    if (spec.empty()) {
        return false;
    }
    else if (spec[0] == '#') {
        // #rgb
        // #rrggbb
        // #rrrgggbbb
        // #rrrrggggbbbb

        const char *startptr = spec.c_str() + 1;
        char *endptr;
        long long val = strtol(startptr, &endptr, 16);
        if (*endptr || val < 0)
            return false;

        switch (spec.size()) {
        case 4:
            *result = (val & 0xf00) << 12 | (val & 0xf0) << 8 | (val & 0xf) << 4;
            return true;
        case 7:
            *result = val;
            return true;
        case 10:
            *result = (val & 0xff0000000) >> 12 | (val & 0xff0000) >> 8 |
                (val & 0xff0) >> 4;
            return true;
        case 13:
            *result = (val & 0xff0000000000) >> 24 | (val & 0xff000000) >> 16 |
                (val & 0xff00) >> 8;
            return true;
        default:
            return false;
        }
    }
    else if (spec.compare(0, 4, "rgb:") == 0) {
        // rgb:r/g/b
        // rgb:rr/gg/bb
        // rgb:rrr/ggg/bbb
        // rgb:rrrr/gggg/bbbb

        switch (spec.size()) {
        case 9:
            if (spec[5] != '/' || spec[7] != '/')
                return false;
            spec.erase(7, 1);
            spec.erase(5, 1);
            break;
        case 12:
            if (spec[6] != '/' || spec[9] != '/')
                return false;
            spec.erase(9, 1);
            spec.erase(6, 1);
            break;
        case 15:
            if (spec[7] != '/' || spec[11] != '/')
                return false;
            spec.erase(11, 1);
            spec.erase(7, 1);
            break;
        case 18:
            if (spec[8] != '/' || spec[13] != '/')
                return false;
            spec.erase(13, 1);
            spec.erase(8, 1);
            break;
        default:
            return false;
        }

        const char *startptr = spec.c_str() + 4;
        char *endptr;
        long long val = strtol(startptr, &endptr, 16);
        if (*endptr || val < 0)
            return false;

        switch (spec.size()) {
        case 7:
            *result = (val & 0xf00) << 12 | (val & 0xf00) << 8 |
                (val & 0xf0) << 8 | (val & 0xf0) << 4 |
                (val & 0xf) << 4 | (val & 0xf);
            return true;
        case 10:
            *result = val;
            return true;
        case 13:
            *result = (val & 0xff0000000) >> 12 | (val & 0xff0000) >> 8 |
                (val & 0xff0) >> 4;
            return true;
        case 16:
            *result = (val & 0xff0000000000) >> 24 | (val & 0xff000000) >> 16 |
                (val & 0xff00) >> 8;
            return true;
        default:
            return false;
        }
    }

    return false;
}

void
XTermEmulator::osc4(const string &str, int offset, int max)
{
    size_t cur = 0;
    unsigned color;
    bool done = false;

    while (!done) {
        // Find the next number and spec substrings
        size_t idx = str.find(';', cur);
        if (idx == string::npos)
            break;

        string colorNum = str.substr(cur, idx - cur);
        cur = idx + 1;
        idx = str.find(';', cur);

        if (idx == string::npos) {
            idx = str.size();
            done = true;
        }

        string colorSpec = str.substr(cur, idx - cur);
        cur = idx + 1;

        // Parse the number
        const char *startptr = colorNum.c_str();
        char *endptr;
        long num = strtol(startptr, &endptr, 10);
        if (!*startptr || *endptr || num < 0 || num >= max)
            break;

        num += offset;

        // Parse the spec
        if (colorSpec == "?") {
            char buf[40];
            color = (*m_palette)[num] & PALETTE_COLOR;
            snprintf(buf, sizeof(buf),
                     "\xc2\x9d""4;%ld;rgb:%04x/%04x/%04x%s", num,
                     (color & 0xff0000) >> 16 | (color & 0xff0000) >> 8,
                     (color & 0xff00) >> 8 | (color & 0xff00),
                     (color & 0xff) | (color & 0xff) << 8,
                     m_state.allSequence().back() == 7 ? "\x07" : "\xc2\x9c");
            termReply(buf);
        }
        else if (colorParse(colorSpec, &color)) {
            (*m_palette)[num] = color;
            setAttribute(Tsq::attr_SESSION_PALETTE, m_palette->toString());
        }
    }
}

static string
urlDecode(const string &str)
{
    string result;
    const char *ptr = str.data();

    for (size_t i = 0; i < str.size(); ++i) {
        if (ptr[i] == '%') {
            int c = '?';
            sscanf(ptr + i + 1, "%2x", &c);
            result.push_back(c);
            i += 2;
        } else {
            result.push_back(ptr[i]);
        }
    }

    return result;
}

void
XTermEmulator::osc7(const string &str)
{
    setAttribute(Tsq::attr_SESSION_OSC7, str);
    string url = urlDecode(str);

    if (url.compare(0, 7, "file://") == 0) {
        size_t idx = url.find('/', 7);

        if (idx != string::npos) {
            setAttribute(Tsq::attr_SESSION_PATH,
                         m_sivars[SH_PATH] = url.substr(idx));

            if (idx > 7) {
                string host = url.substr(7, idx - 7);
                idx = host.find('@');
                if (idx != string::npos)
                    setAttribute(Tsq::attr_SESSION_USER,
                                 m_sivars[SH_USER] = host.substr(0, idx));

                setAttribute(Tsq::attr_SESSION_HOST,
                             m_sivars[SH_HOST] = host.substr(idx + 1));
            }
        }
    }
}

void
XTermEmulator::osc8(string &str, const char *type)
{
    size_t idx = str.find(';');
    if (idx == string::npos || idx == str.size() - 1) {
        m_attributes.flags &= ~Tsq::Hyperlink;
        m_attributes.link = INVALID_REGION_ID;
    } else {
        auto *r = new Region(Tsq::RegionContent);
        r->endRow = r->startRow = m_screen->offset() + m_screen->bounds().bottom();
        r->endCol = r->startCol = 0;
        r->attributes[Tsq::attr_CONTENT_TYPE] = type;
        r->attributes[Tsq::attr_CONTENT_URI] = str.substr(idx + 1);

        str.erase(idx);

        while (!str.empty()) {
            idx = str.find('=');
            if (idx == 0 || idx == string::npos)
                break;

            string key = str.substr(0, idx);
            str.erase(0, idx + 1);

            idx = str.find(':');
            if (idx == string::npos)
                idx = str.size();

            r->attributes.emplace(std::move(key), str.substr(0, idx));
            str.erase(0, idx + 1);
        }

        m_screen->buffer()->addRegion(r);

        m_attributes.flags |= Tsq::Hyperlink;
        m_attributes.link = r->id;
    }
}

void
XTermEmulator::osc10(const string &str, int start)
{
    size_t cur = 0;
    unsigned color;
    bool done = false;

    while (start < 20 && !done) {
        size_t idx = str.find(';', cur);

        if (idx == string::npos) {
            idx = str.size();
            done = true;
        }

        string colorSpec = str.substr(cur, idx - cur);
        cur = idx + 1;

        // Parse the spec
        if (colorSpec == "?") {
            char buf[32];
            color = (*m_palette)[250 + start] & PALETTE_COLOR;
            snprintf(buf, sizeof(buf),
                     "\xc2\x9d""%d;rgb:%04x/%04x/%04x%s", start,
                     (color & 0xff0000) >> 16 | (color & 0xff0000) >> 8,
                     (color & 0xff00) >> 8 | (color & 0xff00),
                     (color & 0xff) | (color & 0xff) << 8,
                     m_state.allSequence().back() == 7 ? "\x07" : "\xc2\x9c");
            termReply(buf);
        }
        else if (colorParse(colorSpec, &color)) {
            (*m_palette)[250 + start] = color;
            setAttribute(Tsq::attr_SESSION_PALETTE, m_palette->toString());
        }

        ++start;
    }
}

void
XTermEmulator::osc52(const string &str)
{
    size_t idx = str.find(';');
    if (idx == string::npos)
        return;

    string spec = str.substr(0, idx);
    string data = str.substr(idx + 1);

    for (auto i = spec.begin(); i != spec.end(); )
        if (strchr("cps01234567", *i))
            ++i;
        else
            i = spec.erase(i);

    if (data == "?") {
        data = "\xc2\x9d""52;";
        data.append(spec + ';');
        if (!spec.empty()) {
            // Need to unpack and then repack the data
            string key = Tsq::attr_CLIPBOARD_PREFIX + spec.front();
            string value = m_parent->getAttribute(key);
            if (unbase64_inplace(value)) {
                string encoded;
                base64(value.data(), value.size(), encoded);
                data.append(encoded);
            }
        }
        data.append(m_state.allSequence().back() == 7 ? "\x07" : "\xc2\x9c");
        termReply(data.data());
    }
    else if (unbase64_validate(data)) {
        for (char c: spec) {
            string key = Tsq::attr_CLIPBOARD_PREFIX + c;
            setAttribute(key, data);
        }
    }
}

void
XTermEmulator::osc104(const string &str, int offset, int max)
{
    TermPalette palette(m_parent->getAttribute(Tsq::attr_PREF_PALETTE));
    size_t cur = 0;
    bool done = false;

    if (str.empty()) {
        done = true;
        for (int i = 0; i < 260; ++i)
            (*m_palette)[i] = palette[i];
    }

    while (!done) {
        // Find the next number
        size_t idx = str.find(';', cur);
        if (idx == string::npos) {
            idx = str.size();
            done = true;
        }

        string colorNum = str.substr(cur, idx - cur);
        cur = idx + 1;

        // Parse the number
        const char *startptr = colorNum.c_str();
        char *endptr;
        long num = strtol(startptr, &endptr, 10);
        if (!*startptr || *endptr || num < 0 || num >= max)
            break;

        num += offset;
        (*m_palette)[num] = palette[num];
    }

    setAttribute(Tsq::attr_SESSION_PALETTE, m_palette->toString());
}

void
XTermEmulator::osc110(int num)
{
    TermPalette palette(m_parent->getAttribute(Tsq::attr_PREF_PALETTE));
    num += 150;
    (*m_palette)[num] = palette[num];
    setAttribute(Tsq::attr_SESSION_PALETTE, m_palette->toString());
}

inline void
XTermEmulator::osc133()
{
    removeAttribute(Tsq::attr_COMMAND);
    m_attributes.flags &= ~(Tsq::Prompt|Tsq::Command);

    if (m_altActive || m_state.var(1).empty())
        return;

    Codestring &cs = m_state.varref(1);

    switch(cs.front()) {
    case 'A':
        if (m_promptNewline && !m_screen->cursorAtLeft()) {
            carriageReturn();
            lineFeed();
        }
        m_attributes.flags &= ~Tsq::All;
        m_attributes.flags |= Tsq::Prompt;
        m_screen->beginPromptRegion();
        break;
    case 'B':
        m_attributes.flags |= Tsq::Command;
        m_screen->beginCommandRegion();
        break;
    case 'C':
        m_screen->beginOutputRegion(m_sivars);
        break;
    case 'D':
        int code;
        if (cs.size() > 1 && cs.at(1) == ';') {
            cs.pop_front(2);
            code = cs.toUInt();
        } else {
            code = -1;
        }
        m_screen->endOutputRegion(code);
        break;
    }
}

void
XTermEmulator::osc513(string &str)
{
    bool found;
    string value = m_parent->getAttribute(str, &found);
    str.insert(0, "\xc2\x9d""514;");
    if (found) {
        string encoded;
        base64(value.data(), value.size(), encoded);
        str.push_back('=');
        str.append(encoded);
    }
    str.append(m_state.allSequence().back() == 7 ? "\x07" : "\xc2\x9c");
    termReply(str.data());
}

void
XTermEmulator::osc514(string &str)
{
    static const std::set<std::string> s_settableAttributes = {
        TSQ_ATTR_SESSION_ICON,
        TSQ_ATTR_SESSION_BADGE,
        TSQ_ATTR_SESSION_LAYOUT,
        TSQ_ATTR_SESSION_FILLS,
    };

    size_t idx = str.find('=');
    string key;

    if (idx != string::npos) {
        key = str.substr(0, idx);
        str.erase(0, idx + 1);
    } else {
        key = std::move(str);
    }

    if (s_settableAttributes.count(key)) {
        if (idx == string::npos) {
            removeAttribute(key);
        } else if (unbase64_inplace_utf8(str)) {
            setAttribute(key, str);
        }
    }
}

void
XTermEmulator::osc1337File(string &str)
{
    size_t idx = str.find(':');
    if (idx == string::npos || m_altActive)
        return;

    string params = str.substr(5, idx - 5);
    AttributeMap attributes;
    contentid_t id;
    bool isinline = false;
    long w = 0, h = 0;

    while (!params.empty()) {
        size_t pos = params.find('=');
        if (pos == 0 || pos == string::npos)
            return;

        string key = params.substr(0, pos);
        params.erase(0, pos + 1);

        pos = params.find(';');
        if (pos == string::npos)
            pos = params.size();

        string &val = (attributes[key] = params.substr(0, pos));
        params.erase(0, pos + 1);

        if (key == Tsq::attr_CONTENT_INLINE)
            isinline = val == "1"s;
        if (key == Tsq::attr_CONTENT_NAME && !unbase64_inplace_utf8(val))
            return;
    }

    if (!unbase64_inplace_hash(str, idx + 1, id))
        return;

    // Determine region height
    if (isinline) {
        string hstr = attributes[Tsq::attr_CONTENT_HEIGHT];
        const char *startptr = hstr.c_str();
        char *endptr;
        h = strtol(startptr, &endptr, 10);

        if (hstr.empty()) {
            h = -1;
        } else if (*endptr) {
            if (!strcmp(endptr, "%"))
                h = (m_screen->bounds().height() * h) / 100;
            else
                h = -1;
        }
        if (h < 0)
            h = m_screen->bounds().height() / 3;
        if (h == 0)
            return;

        // Determine region width
        string wstr = attributes[Tsq::attr_CONTENT_WIDTH];
        startptr = wstr.c_str();
        w = strtol(startptr, &endptr, 10);

        if (wstr.empty()) {
            w = -1;
        } else if (*endptr) {
            if (!strcmp(endptr, "%"))
                w = (m_screen->bounds().width() * w) / 100;
            else
                w = -1;
        }
        if (w < 0)
            w = m_screen->margins().width() - m_screen->cursor().x();
        if (w <= 0 || m_screen->cursorPastEnd(1))
            return;
    }

    // Begin region
    attributes[Tsq::attr_CONTENT_ID] = std::to_string(id);
    attributes[Tsq::attr_CONTENT_SIZE] = std::to_string(str.size());

    auto i = m_contentMap.find(id);
    if (i == m_contentMap.end()) {
        m_contentMap.emplace(id, new string(std::move(str)));
    } else {
        ++i->second.refcount;
    }

    Region *region = new Region(Tsq::RegionImage);
    region->attributes = std::move(attributes);
    region->beginAtX(m_buf[0], m_screen);

    if (isinline) {
        // Move cursor
        while (--h)
            lineFeed();

        m_screen->cursorAdvance(w);
    }

    // End region
    region->endAtX(m_buf[0], m_screen);
}

void
XTermEmulator::osc1337(string &str)
{
    if (str.compare(0, 11, "CurrentDir=") == 0) {
        setAttribute(Tsq::attr_SESSION_PATH,
                     m_sivars[SH_PATH] = str.substr(11));
    }
    else if (str.compare(0, 11, "RemoteHost=") == 0) {
        size_t idx = str.find('@');
        if (idx != string::npos) {
            setAttribute(Tsq::attr_SESSION_USER,
                         m_sivars[SH_USER] = str.substr(11, idx - 11));
            setAttribute(Tsq::attr_SESSION_HOST,
                         m_sivars[SH_HOST] = str.substr(idx + 1));
        }
    }
    else if (str.compare(0, 11, "SetUserVar=") == 0) {
        str.replace(0, 11, Tsq::attr_USER_PREFIX);

        size_t idx = str.find('=');
        if (idx != string::npos) {
            string key = str.substr(0, idx);
            str.erase(0, idx + 1);

            if (unbase64_inplace_utf8(str))
                setAttribute(key, str);
        }
    }
    else if (str.compare(0, 15, "SetBadgeFormat=") == 0) {
        str.erase(0, 15);

        if (unbase64_inplace_utf8(str))
            setAttribute(Tsq::attr_SESSION_BADGE, str);
    }
    else if (str.compare(0, 24, "ShellIntegrationVersion=") == 0) {
        str.erase(0, 24);
        size_t idx = str.find(';');
        if (idx == string::npos) {
            setAttribute(Tsq::attr_SESSION_SIVERSION, str);
        } else {
            setAttribute(Tsq::attr_SESSION_SIVERSION, str.substr(0, idx));
            str.erase(0, idx + 1);

            while (!str.empty()) {
                idx = str.find('=');
                if (idx == 0 || idx == string::npos)
                    break;

                string key = Tsq::attr_SESSION_SIPREFIX + str.substr(0, idx);
                str.erase(0, idx + 1);

                idx = str.find(';');
                if (idx == string::npos)
                    idx = str.size();

                setAttribute(key, str.substr(0, idx));
                str.erase(0, idx + 1);
            }
        }
    }
    else if (str.compare(0, 5, "File=") == 0) {
        osc1337File(str);
    }
    else {
        m_state.dumpError("Unimplemented osc1337 command", false);
    }
}

void
XTermEmulator::oscMain()
{
    const Codestring &text = m_state.var(1);
    int arg = m_state.var(0).toUInt();

    switch (arg) {
    case 0:
    case 1:
    case 2:
        if (!text.empty())
            osc0(m_state.varref(1).rstr(), arg);
        break;
    case 3:
        osc3(text.str());
        break;
    case 4:
        osc4(text.str(), 0, 260);
        break;
    case 5:
        osc4(text.str(), 256, 4);
        break;
    case 6:
        setAttribute(Tsq::attr_SESSION_OSC6, text.str());
        break;
    case 7:
        osc7(text.str());
        break;
    case 8:
        if (!text.empty())
            osc8(m_state.varref(1).rstr(), "8");
        break;
    case 9:
        // unsupported
        break;
    case 10:
    case 11:
    case 12:
    case 13:
    case 14:
    case 15:
    case 16:
    case 17:
    case 18:
    case 19:
        osc10(text.str(), m_state.var(0).toUInt());
        break;
    case 46:
    case 50:
    case 51:
        // unsupported
        break;
    case 52:
        osc52(text.str());
        break;
    case 104:
        osc104(text.str(), 0, 260);
        break;
    case 105:
        osc104(text.str(), 256, 4);
        break;
    case 110:
    case 111:
    case 112:
    case 113:
    case 114:
    case 115:
    case 116:
    case 117:
    case 118:
    case 119:
        osc110(m_state.var(0).toUInt());
        break;
    case 133:
    case 1333: // seems to happen in an undersize terminal
        osc133();
        break;
    case 511:
        termReply(m_parent->termCommand(m_state.allSequence()).data());
        break;
    case 512:
        m_parent->termData(text);
        break;
    case 513:
        if (!text.empty())
            osc513(m_state.varref(1).rstr());
        break;
    case 514:
        if (!text.empty())
            osc514(m_state.varref(1).rstr());
        break;
    case 515:
        if (!text.empty())
            osc8(m_state.varref(1).rstr(), "515");
        break;
    case 777:
        // unsupported
        break;
    case 1337:
        if (!text.empty())
            osc1337(m_state.varref(1).rstr());
        break;
    default:
        m_state.dumpError("Unimplemented os command", false);
        break;
    }
}
