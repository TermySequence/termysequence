// Copyright © 2018 TermySequence LLC
//
// SPDX-License-Identifier: GPL-2.0-only

#include "common.h"
#include "app/attr.h"
#include "note.h"
#include "region.h"
#include "term.h"
#include "server.h"
#include "mark.h"

#include <QDateTime>

void
TermNote::recolor()
{
    int bgi, fgi;
    TermMark::getNoteColors(bgi, fgi);

    const TermPalette &palette = term->palette();

    termBg = term->bg();
    termFg = term->fg();

    markBg = palette.at(bgi);
    if (PALETTE_IS_DISABLED(markBg))
        markBg = termBg;

    markFg = palette.at(fgi);
    if (PALETTE_IS_DISABLED(markFg))
        markFg = termFg;
}

void
TermNote::update(const Region *r)
{
    attributes = r->attributes;

    QDateTime date = QDateTime::fromMSecsSinceEpoch(started);
    attributes[g_attr_TSQT_REGION_STARTED] = date.toString();
    attributes[g_attr_TSQT_REGION_ROW] = QString::number(r->startRow);

    attributes[g_attr_TSQT_REGION_MARK] = TermMark::getNoteText(
        attributes.value(g_attr_REGION_NOTECHAR));

    recolor();
}

void
TermNote::populate(const TermInstance *t, const Region *r)
{
    term = t;
    termId = t->id();
    serverId = t->server()->id();
    unicoding = t->unicodingPtr();
    region = r->id();

    update(r);
}
