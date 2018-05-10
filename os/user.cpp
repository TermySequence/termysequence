// Copyright © 2018 TermySequence LLC
//
// SPDX-License-Identifier: GPL-2.0-only

#include "common.h"
#include "os/user.h"

#include <pwd.h>
#include <grp.h>

std::string
osUserName(uid_t id)
{
    char buf[3072];
    struct passwd pwd, *ret;

    getpwuid_r(id, &pwd, buf, sizeof(buf), &ret);

    if (ret != NULL) {
        return pwd.pw_name;
    } else {
        return std::to_string(id);
    }
}

std::string
osGroupName(gid_t id)
{
    char buf[3072];
    struct group grp, *ret;

    getgrgid_r(id, &grp, buf, sizeof(buf), &ret);

    if (ret != NULL) {
        return grp.gr_name;
    } else {
        return std::to_string(id);
    }
}
