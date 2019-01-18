// Copyright © 2018 TermySequence LLC
//
// SPDX-License-Identifier: GPL-2.0-only

#include "common.h"
#include "os/attr.h"

bool
osFallbackIdentity(Tsq::Uuid &result)
{
    result.generate();
    return false;
}
