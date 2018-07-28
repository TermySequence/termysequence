// Copyright © 2018 TermySequence LLC
//
// SPDX-License-Identifier: GPL-2.0-only

#include "common.h"
#include "parsemap.h"
#include "os/attr.h"

void
parseStringMap(Tsq::ProtocolUnmarshaler &unm, StringMap &map)
{
    while (unm.remainingLength()) {
        std::string key = unm.parseString();

        if (key.empty())
            break;

        map[key] = unm.parseString();
    }
}

void
parseUtf8Map(Tsq::ProtocolUnmarshaler &unm, StringMap &map)
{
    unm.validateAsUtf8();
    parseStringMap(unm, map);
}

StringMap
parseTermMap(Tsq::ProtocolUnmarshaler &unm)
{
    StringMap result;

    unm.validateAsUtf8();

    while (unm.remainingLength()) {
        std::string key = unm.parseString();

        if (key.empty())
            break;

        if (!osRestrictedTermAttribute(key, true))
            result[key] = unm.parseString();
    }

    return result;
}
