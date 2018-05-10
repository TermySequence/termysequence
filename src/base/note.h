// Copyright © 2018 TermySequence LLC
//
// SPDX-License-Identifier: GPL-2.0-only

#pragma once

#include "app/attrbase.h"
#include "lib/types.h"
#include "lib/uuid.h"

#include <QRgb>
#include <memory>

namespace Tsq { class Unicoding; }
class TermInstance;
class Region;

struct TermNote
{
    int64_t started;

    const TermInstance *term;
    Tsq::Uuid termId;
    Tsq::Uuid serverId;

    std::shared_ptr<Tsq::Unicoding> unicoding;
    regionid_t region;
    int fade;

    QRgb termBg, termFg;
    QRgb markBg, markFg;

    AttributeMap attributes;

public:
    inline TermNote(int64_t s): started(s), term(nullptr), fade(0) {}

    void populate(const TermInstance *term, const Region *region);
    void update(const Region *region);
    void recolor();

    inline bool operator<(const TermNote &o) const
    { return started < o.started; }
};
