// Copyright © 2018 TermySequence LLC
//
// SPDX-License-Identifier: GPL-2.0-only

#include "common.h"
#include "app/attr.h"
#include "job.h"
#include "region.h"
#include "term.h"
#include "server.h"
#include "mark.h"
#include "modtimes.h"
#include "thumbicon.h"
#include "settings/iconrule.h"

#include <QDateTime>

void
TermJob::recolor(const QString &exitcode)
{
    int bgi, fgi;
    TermMark::getJobColors(exitcode, bgi, fgi);

    const TermPalette &palette = term->palette();

    commandBg = term->bg();
    commandFg = term->fg();

    markBg = palette.at(bgi);
    if (PALETTE_IS_DISABLED(markBg))
        markBg = commandBg;

    markFg = palette.at(fgi);
    if (PALETTE_IS_DISABLED(markFg))
        markFg = commandFg;
}

bool
TermJob::update(Region *r)
{
    attributes = r->attributes;

    QDateTime date = QDateTime::fromMSecsSinceEpoch(started);
    attributes[g_attr_TSQT_REGION_STARTED] = date.toString();
    attributes[g_attr_TSQT_REGION_ROW] = QString::number(r->startRow);

    auto i = attributes.constFind(g_attr_REGION_ENDED);
    if (i != attributes.cend()) {
        duration = i->toLongLong() - started;
        attributes[g_attr_TSQT_REGION_DURATION] = TermModtimes::getTimeString64(duration);
    }

    if (!(r->flags & Tsqt::HaveJobIcon)) {
        // Look up icon renderer for command
        // Also determine if this is an ignored command
        QString str = attributes[g_attr_REGION_COMMAND];
        switch (g_iconrules->findCommand(str)) {
        case 1:
            renderer = ThumbIcon::getRenderer(ThumbIcon::CommandType, str);
            r->renderer = renderer;
            break;
        case -1:
            r->flags |= Tsqt::IgnoredCommand;
            break;
        }
        r->flags |= Tsqt::HaveJobIcon;
    }

    QString exitcode = attributes.value(g_attr_REGION_EXITCODE);
    attributes[g_attr_TSQT_REGION_MARK] = TermMark::getJobText(exitcode);

    // Return true if this is the first update with an exit code
    bool rc = !exitcode.isEmpty() && !(r->flags & Tsqt::HaveJobExitCode);
    if (rc) {
        r->flags |= Tsqt::HaveJobExitCode;
    }

    recolor(exitcode);
    return rc;
}

void
TermJob::populate(const TermInstance *t, Region *r)
{
    term = t;
    termId = t->id();
    serverId = t->server()->id();
    unicoding = t->unicodingPtr();
    region = r->id();

    update(r);
}
