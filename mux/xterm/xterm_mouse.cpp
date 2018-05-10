// Copyright © 2018 TermySequence LLC
//
// SPDX-License-Identifier: GPL-2.0-only

#include "common.h"
#include "xterm.h"
#include "base/term.h"
#include "base/output.h"

using namespace std;

static string
x10_utf8(uint32_t event, uint32_t x, uint32_t y)
{
    uint8_t button = event & Tsq::MouseButtonMask;
    bool ispress = (event & (Tsq::MouseRelease|Tsq::MouseMotion)) == 0;

    if (ispress && button && button < 4) {
        Codestring data("\x1b[M");
        data.push_back(button + 31);
        data.push_back((x += 33) < 2047 ? x : 2047);
        data.push_back((y += 33) < 2047 ? y : 2047);
        return data.str();
    }
    return string();
}

static string
x10_sgr(uint32_t event, uint32_t x, uint32_t y)
{
    uint8_t button = event & Tsq::MouseButtonMask;
    bool ispress = (event & (Tsq::MouseRelease|Tsq::MouseMotion)) == 0;

    if (ispress && button && button < 4) {
        string data("\x1b[<");
        data += std::to_string(button - 1);
        data += ';';
        data += std::to_string(x + 1);
        data += ';';
        data += std::to_string(y + 1);
        data += 'M';
        return data;
    }
    return string();
}

static string
x10_urxvt(uint32_t event, uint32_t x, uint32_t y)
{
    uint8_t button = event & Tsq::MouseButtonMask;
    bool ispress = (event & (Tsq::MouseRelease|Tsq::MouseMotion)) == 0;

    if (ispress && button && button < 4) {
        string data("\x1b[");
        data += std::to_string(button + 31);
        data += ';';
        data += std::to_string(x + 1);
        data += ';';
        data += std::to_string(y + 1);
        data += 'M';
        return data;
    }
    return string();
}

static string
normal_utf8(uint32_t event, uint32_t x, uint32_t y)
{
    uint8_t button = event & Tsq::MouseButtonMask;
    bool isrelease = event & Tsq::MouseRelease;
    bool ismotion = event & Tsq::MouseMotion;

    if (!ismotion && button) {
        uint32_t code;
        if (button < 4)
            code = isrelease ? 3 : button - 1;
        else if (!isrelease && button < 6)
            code = button + 60;
        else
            goto out;

        if (event & Tsq::MouseShift)
            code |= 4;
        if (event & Tsq::MouseMeta)
            code |= 8;
        if (event & Tsq::MouseControl)
            code |= 16;

        Codestring data("\x1b[M");
        data.push_back(code + 32);
        data.push_back((x += 33) < 2047 ? x : 2047);
        data.push_back((y += 33) < 2047 ? y : 2047);
        return data.str();
    }
out:
    return string();
}

static string
normal_sgr(uint32_t event, uint32_t x, uint32_t y)
{
    uint8_t button = event & Tsq::MouseButtonMask;
    bool isrelease = event & Tsq::MouseRelease;
    bool ismotion = event & Tsq::MouseMotion;

    if (!ismotion && button) {
        uint32_t code;
        if (button < 4)
            code = button - 1;
        else if (!isrelease && button < 6)
            code = button + 60;
        else
            goto out;

        if (event & Tsq::MouseShift)
            code |= 4;
        if (event & Tsq::MouseMeta)
            code |= 8;
        if (event & Tsq::MouseControl)
            code |= 16;

        string data("\x1b[<");
        data += std::to_string(code);
        data += ';';
        data += std::to_string(x + 1);
        data += ';';
        data += std::to_string(y + 1);
        data += isrelease ? 'm' : 'M';
        return data;
    }
out:
    return string();
}

static string
normal_urxvt(uint32_t event, uint32_t x, uint32_t y)
{
    uint8_t button = event & Tsq::MouseButtonMask;
    bool isrelease = event & Tsq::MouseRelease;
    bool ismotion = event & Tsq::MouseMotion;

    if (!ismotion && button) {
        uint32_t code;
        if (button < 4)
            code = isrelease ? 3 : button - 1;
        else if (!isrelease && button < 6)
            code = button + 60;
        else
            goto out;

        if (event & Tsq::MouseShift)
            code |= 4;
        if (event & Tsq::MouseMeta)
            code |= 8;
        if (event & Tsq::MouseControl)
            code |= 16;

        string data("\x1b[");
        data += std::to_string(code + 32);
        data += ';';
        data += std::to_string(x + 1);
        data += ';';
        data += std::to_string(y + 1);
        data += 'M';
        return data;
    }
out:
    return string();
}

static string
button_utf8(uint32_t event, uint32_t x, uint32_t y)
{
    uint8_t button = event & Tsq::MouseButtonMask;
    bool isrelease = event & Tsq::MouseRelease;
    bool ismotion = event & Tsq::MouseMotion;

    if (button) {
        uint32_t code;
        if (button < 4)
            code = isrelease ? 3 : button - 1;
        else if (!isrelease && button < 6)
            code = button + 60;
        else
            goto out;

        if (event & Tsq::MouseShift)
            code |= 4;
        if (event & Tsq::MouseMeta)
            code |= 8;
        if (event & Tsq::MouseControl)
            code |= 16;
        if (ismotion)
            code |= 32;

        Codestring data("\x1b[M");
        data.push_back(code + 32);
        data.push_back((x += 33) < 2047 ? x : 2047);
        data.push_back((y += 33) < 2047 ? y : 2047);
        return data.str();
    }
out:
    return string();
}

static string
button_sgr(uint32_t event, uint32_t x, uint32_t y)
{
    uint8_t button = event & Tsq::MouseButtonMask;
    bool isrelease = event & Tsq::MouseRelease;
    bool ismotion = event & Tsq::MouseMotion;

    if (button) {
        uint32_t code;
        if (button < 4)
            code = button - 1;
        else if (!isrelease && button < 6)
            code = button + 60;
        else
            goto out;

        if (event & Tsq::MouseShift)
            code |= 4;
        if (event & Tsq::MouseMeta)
            code |= 8;
        if (event & Tsq::MouseControl)
            code |= 16;
        if (ismotion)
            code |= 32;

        string data("\x1b[<");
        data += std::to_string(code);
        data += ';';
        data += std::to_string(x + 1);
        data += ';';
        data += std::to_string(y + 1);
        data += isrelease ? 'm' : 'M';
        return data;
    }
out:
    return string();
}

static string
button_urxvt(uint32_t event, uint32_t x, uint32_t y)
{
    uint8_t button = event & Tsq::MouseButtonMask;
    bool isrelease = event & Tsq::MouseRelease;
    bool ismotion = event & Tsq::MouseMotion;

    if (button) {
        uint32_t code;
        if (button < 4)
            code = isrelease ? 3 : button - 1;
        else if (!isrelease && button < 6)
            code = button + 60;
        else
            goto out;

        if (event & Tsq::MouseShift)
            code |= 4;
        if (event & Tsq::MouseMeta)
            code |= 8;
        if (event & Tsq::MouseControl)
            code |= 16;
        if (ismotion)
            code |= 32;

        string data("\x1b[");
        data += std::to_string(code + 32);
        data += ';';
        data += std::to_string(x + 1);
        data += ';';
        data += std::to_string(y + 1);
        data += 'M';
        return data;
    }
out:
    return string();
}

static string
any_utf8(uint32_t event, uint32_t x, uint32_t y)
{
    uint8_t button = event & Tsq::MouseButtonMask;
    bool isrelease = event & Tsq::MouseRelease;
    bool ismotion = event & Tsq::MouseMotion;
    uint32_t code;

    if (button == 0)
        code = 3;
    else if (button < 4)
        code = isrelease ? 3 : button - 1;
    else if (!isrelease && button < 6)
        code = button + 60;
    else
        return string();

    if (event & Tsq::MouseShift)
        code |= 4;
    if (event & Tsq::MouseMeta)
        code |= 8;
    if (event & Tsq::MouseControl)
        code |= 16;
    if (ismotion)
        code |= 32;

    Codestring data("\x1b[M");
    data.push_back(code + 32);
    data.push_back((x += 33) < 2047 ? x : 2047);
    data.push_back((y += 33) < 2047 ? y : 2047);
    return data.str();
}

static string
any_sgr(uint32_t event, uint32_t x, uint32_t y)
{
    uint8_t button = event & Tsq::MouseButtonMask;
    bool isrelease = event & Tsq::MouseRelease;
    bool ismotion = event & Tsq::MouseMotion;
    uint32_t code;

    if (button == 0)
        code = 3;
    else if (button < 4)
        code = button - 1;
    else if (!isrelease && button < 6)
        code = button + 60;
    else
        return string();

    if (event & Tsq::MouseShift)
        code |= 4;
    if (event & Tsq::MouseMeta)
        code |= 8;
    if (event & Tsq::MouseControl)
        code |= 16;
    if (ismotion)
        code |= 32;

    string data("\x1b[<");
    data += std::to_string(code);
    data += ';';
    data += std::to_string(x + 1);
    data += ';';
    data += std::to_string(y + 1);
    data += isrelease ? 'm' : 'M';
    return data;
}

static string
any_urxvt(uint32_t event, uint32_t x, uint32_t y)
{
    uint8_t button = event & Tsq::MouseButtonMask;
    bool isrelease = event & Tsq::MouseRelease;
    bool ismotion = event & Tsq::MouseMotion;
    uint32_t code;

    if (button == 0)
        code = 3;
    else if (button < 4)
        code = isrelease ? 3 : button - 1;
    else if (!isrelease && button < 6)
        code = button + 60;
    else
        return string();

    if (event & Tsq::MouseShift)
        code |= 4;
    if (event & Tsq::MouseMeta)
        code |= 8;
    if (event & Tsq::MouseControl)
        code |= 16;
    if (ismotion)
        code |= 32;

    string data("\x1b[");
    data += std::to_string(code + 32);
    data += ';';
    data += std::to_string(x + 1);
    data += ';';
    data += std::to_string(y + 1);
    data += 'M';
    return data;
}

static string
todo_func(uint32_t, uint32_t, uint32_t)
{
    return string();
}

typedef string (*MouseEncodeFunc)(uint32_t event, uint32_t x, uint32_t y);
static const MouseEncodeFunc s_encoders[15] = {
    &x10_utf8,  &normal_utf8,  &todo_func, &button_utf8,  &any_utf8,
    &x10_sgr,   &normal_sgr,   &todo_func, &button_sgr,   &any_sgr,
    &x10_urxvt, &normal_urxvt, &todo_func, &button_urxvt, &any_urxvt,
};

bool
XTermEmulator::termMouse(unsigned event, unsigned x, unsigned y)
{
    bool valid, rc;

    {
        unsigned encoder;
        string data;
        TermInstance::StateLock slock(m_parent, false);

        switch (m_flags & Tsq::MouseModeMask) {
        case Tsq::X10MouseMode:
            encoder = 0;
            break;
        case Tsq::NormalMouseMode:
            encoder = 1;
            break;
        case Tsq::HighlightMouseMode:
            encoder = 2;
            break;
        case Tsq::ButtonEventMouseMode:
            encoder = 3;
            break;
        case Tsq::AnyEventMouseMode:
            encoder = 4;
            break;
        default:
            return true;
        }
        switch (m_flags & Tsq::ExtMouseModeMask) {
        case Tsq::UrxvtExtMouseMode:
            encoder += 10;
            break;
        case Tsq::SgrExtMouseMode:
            encoder += 5;
            break;
        default:
            break;
        }

        valid = x < m_screen->width() && y < m_screen->height();
        data = (*s_encoders[encoder])(event, x, y);

        if (valid && !data.empty())
            rc = m_parent->output()->submitData(std::move(data));
        else
            rc = true;
    }

    if (valid)
        m_parent->sendWork(TermMoveMouse, x, y);

    return rc;
}
