// Copyright © 2018 TermySequence LLC
//
// SPDX-License-Identifier: GPL-2.0-only

#include "attr.h"

static const IconRule s_defaultIconRules[] = {
    { VarVerbSet, g_attr_PEER, "", "term-link" },

    { VarVerbIs, g_attr_PROC_COMM, "bash", "none" },
    { VarVerbIs, g_attr_PROC_COMM, "zsh", "none" },
    { VarVerbIs, g_attr_PROC_COMM, "ksh", "none" },
    { VarVerbIs, g_attr_PROC_COMM, "csh", "none" },
    { VarVerbIs, g_attr_PROC_COMM, "dash", "none" },
    { VarVerbIs, g_attr_PROC_COMM, "fish", "none" },
    { VarVerbIs, g_attr_PROC_COMM, "tcsh", "none" },
    { VarVerbIs, g_attr_PROC_COMM, "sh", "none" },

    { VarVerbIs, g_attr_PROC_COMM, "vim", "generic-editor" },
    { VarVerbIs, g_attr_PROC_COMM, "vi", "generic-editor" },
    { VarVerbIs, g_attr_PROC_COMM, "view", "generic-editor" },
    { VarVerbIs, g_attr_PROC_COMM, "emacs", "generic-editor" },
    { VarVerbIs, g_attr_PROC_COMM, "nano", "generic-editor" },

    { VarVerbIs, g_attr_PROC_COMM, "git", "git" },
    { VarVerbStarts, g_attr_PROC_COMM, "git-", "git" },

    { VarVerbIs, g_attr_PROC_COMM, "make", "generic-build" },
    { VarVerbIs, g_attr_PROC_COMM, "gcc", "generic-build" },
    { VarVerbIs, g_attr_PROC_COMM, "g++", "generic-build" },
    { VarVerbIs, g_attr_PROC_COMM, "clang", "generic-build" },
    { VarVerbIs, g_attr_PROC_COMM, "clang++", "generic-build" },

    { VarVerbIs, g_attr_PROC_COMM, "gdb", "generic-debug" },
    { VarVerbIs, g_attr_PROC_COMM, "lldb", "generic-debug" },
    { VarVerbIs, g_attr_PROC_COMM, "coredumpctl", "generic-debug" },

    { VarVerbIs, g_attr_PROC_COMM, "ls", "generic-files" },
    { VarVerbIs, g_attr_PROC_COMM, "find", "generic-files" },
    { VarVerbIs, g_attr_PROC_COMM, "rm", "generic-files" },
    { VarVerbIs, g_attr_PROC_COMM, "cp", "generic-files" },
    { VarVerbIs, g_attr_PROC_COMM, "mv", "generic-files" },

    { VarVerbIs, g_attr_PROC_COMM, "grep", "generic-find" },

    { VarVerbIs, g_attr_PROC_COMM, "ssh", "generic-network" },
    { VarVerbIs, g_attr_PROC_COMM, "scp", "generic-network" },

    { VarVerbIs, g_attr_PROC_COMM, "less", "generic-paginator" },
    { VarVerbIs, g_attr_PROC_COMM, "more", "generic-paginator" },

    { VarVerbStarts, g_attr_PROC_COMM, "python", "generic-script" },
    { VarVerbStarts, g_attr_PROC_COMM, "perl", "generic-script" },
    { VarVerbStarts, g_attr_PROC_COMM, "ruby", "generic-script" },

    { VarVerbIs, g_attr_PROC_COMM, "man", "generic-doc" },
    { VarVerbIs, g_attr_PROC_COMM, "info", "generic-doc" },

    { VarVerbIs, g_attr_PROC_COMM, "ps", "generic-monitor" },
    { VarVerbIs, g_attr_PROC_COMM, "top", "generic-monitor" },
    { VarVerbIs, g_attr_PROC_COMM, "netstat", "generic-monitor" },

    { VarVerbIs, g_attr_PROC_COMM, "ping", "generic-ping" },
    { VarVerbIs, g_attr_PROC_COMM, "nc", "generic-ping" },
    { VarVerbIs, g_attr_PROC_COMM, "socat", "generic-ping" },

    { VarVerbIs, g_attr_PROC_COMM, "rpm", "generic-package" },
    { VarVerbIs, g_attr_PROC_COMM, "dnf", "generic-package" },
    { VarVerbIs, g_attr_PROC_COMM, "yum", "generic-package" },
    { VarVerbStarts, g_attr_PROC_COMM, "apt-", "generic-package" },
    { VarVerbIs, g_attr_PROC_COMM, "dpkg", "generic-package" },

    { VarVerbIs, g_attr_PROC_COMM, "mysql", "generic-database" },
    { VarVerbIs, g_attr_PROC_COMM, "psql", "generic-database" },
    { VarVerbIs, g_attr_PROC_COMM, "mongo", "generic-database" },
};
