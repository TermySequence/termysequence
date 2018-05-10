// Copyright © 2018 TermySequence LLC
//
// SPDX-License-Identifier: GPL-2.0-only

#pragma once

#include "attributemap.h"
#include "lib/wire.h"

extern void
parseStringMap(Tsq::ProtocolUnmarshaler &unm, AttributeMap &map);

extern void
parseUtf8Map(Tsq::ProtocolUnmarshaler &unm, AttributeMap &map);

extern AttributeMap
parseTermMap(Tsq::ProtocolUnmarshaler &unm);
