// Copyright © 2018 TermySequence LLC
//
// SPDX-License-Identifier: GPL-2.0-only

#include "common.h"
#include "app/attr.h"
#include "noteinline.h"
#include "term.h"
#include "buffers.h"
#include "scrollport.h"
#include "thumbicon.h"
#include "mainwindow.h"
#include "mark.h"

#include <QMimeData>

InlineNote::InlineNote(TermWidget *parent) :
    InlineBase(Tsqt::RegionUser, parent)
{
    m_renderer = ThumbIcon::getRenderer(ThumbIcon::SemanticType, L("note"));
}

void
InlineNote::setRegion(const Region *region)
{
    m_region = region;
    setToolTip(TermMark::getNoteTooltip(region, m_term));
    m_renderMask |= AlwaysOn;
    m_borderMask |= AlwaysOn;

    column_t startX = m_term->buffers()->xByPos(region->startRow, region->startCol);
    column_t endX;

    if (region->endRow != region->startRow || region->endCol == INVALID_COLUMN)
        endX = m_scrollport->width();
    else
        endX = m_term->buffers()->xByPos(region->endRow, region->endCol);

    setBounds(startX, 0, endX - startX, 1);
    moveIcon();
}

QMenu *
InlineNote::getPopup(MainWindow *window)
{
    return window->getNotePopup(m_term, m_region->id(), window);
}

void
InlineNote::getDragData(QMimeData *data, QDrag *)
{
    data->setText(m_region->attributes.value(g_attr_REGION_NOTETEXT));
}

void
InlineNote::open()
{
}
