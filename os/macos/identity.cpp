// Copyright © 2018 TermySequence LLC
//
// SPDX-License-Identifier: GPL-2.0-only

#include "common.h"
#include "os/attr.h"
#include "config.h"

#include <unistd.h>
#include <uuid/uuid.h>

void
osFallbackIdentity(Tsq::Uuid &result)
{
    struct timespec ts = {
        .tv_sec = ATTRIBUTE_SCRIPT_TIMEOUT / 1000,
        .tv_nsec = (ATTRIBUTE_SCRIPT_TIMEOUT % 1000) * 1000000
    };

    if (gethostuuid((unsigned char *)result.buf, &ts) != 0)
        result.generate();
}
