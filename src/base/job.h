// Copyright © 2018 TermySequence LLC
//
// SPDX-License-Identifier: GPL-2.0-only

#pragma once

#include "app/attrbase.h"
#include "lib/types.h"
#include "lib/uuid.h"

#include <QRgb>
#include <memory>

QT_BEGIN_NAMESPACE
class QSvgRenderer;
QT_END_NAMESPACE
namespace Tsq { class Unicoding; }
class TermInstance;
class Region;

struct TermJob
{
    int64_t started;
    int duration = -1;

    const TermInstance *term = nullptr;
    Tsq::Uuid termId;
    Tsq::Uuid serverId;

    std::shared_ptr<Tsq::Unicoding> unicoding;
    regionid_t region;
    int fade = 0;

    QRgb commandBg, commandFg;
    QRgb markBg, markFg;

    AttributeMap attributes;
    QSvgRenderer *renderer = nullptr;

public:
    inline TermJob(int64_t s): started(s) {}

    void populate(const TermInstance *term, Region *region);
    bool update(Region *region);
    void recolor(const QString &exitcode);

    inline QRgb exitColor() const
    { return (markFg != commandFg) ? markFg : markBg; }

    inline bool operator<(const TermJob &o) const
    { return started < o.started; }
};
