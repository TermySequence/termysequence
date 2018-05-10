// Copyright © 2018 TermySequence LLC
//
// SPDX-License-Identifier: GPL-2.0-only

#pragma once

#define REG_SETTER_BODY(var) \
    if (m_ ## var != var) { \
        m_ ## var = var; \
        emit settingChanged(#var, var); \
    }

#define REG_SETTER(func, var, type) \
void func(type var) { \
    REG_SETTER_BODY(var) \
}

#define NOTIFY_SETTER_BODY(var) \
    if (m_ ## var != var) { \
        m_ ## var = var; \
        emit var ## Changed(var); \
        emit settingChanged(#var, var); \
    }

#define NOTIFY_SETTER(func, var, type) \
void func(type var) { \
    NOTIFY_SETTER_BODY(var) \
}

#define SIG_SETTER_BODY(var, sig) \
    if (m_ ## var != var) { \
        m_ ## var = var; \
        emit sig(); \
        emit settingChanged(#var, var); \
    }

#define SIG_SETTER(func, var, type, sig) \
void func(type var) { \
    SIG_SETTER_BODY(var, sig) \
}

#define SIGARG_SETTER_BODY(var, sig, arg) \
    if (m_ ## var != var) { \
        m_ ## var = var; \
        emit sig(arg); \
        emit settingChanged(#var, var); \
    }

#define SIGARG_SETTER(func, var, type, sig, arg) \
void func(type var) { \
    SIGARG_SETTER_BODY(var, sig, arg) \
}

#define ENUM_SETTER(func, var, max) \
void func(int var) { \
    if (var < 0 || var >= max) { \
        var = 0; \
    } \
    REG_SETTER_BODY(var) \
}

#define REPORT_SETTER_BODY(var) \
    if (m_ ## var != var) { \
        m_ ## var = var; \
        emit settingChanged(#var, var); \
        reportUpdate(m_row); \
    }

#define REPORT_SETTER(func, var, type) \
void func(type var) { \
    REPORT_SETTER_BODY(var) \
}

#define DIRECT_SETTER(func, var, type) \
void func(type var) { \
    if (m_ ## var != var) { \
        m_ ## var = var; \
        saveSetting(#var, var); \
    } \
}
