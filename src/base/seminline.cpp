// Copyright © 2018 TermySequence LLC
//
// SPDX-License-Identifier: GPL-2.0-only

#include "common.h"
#include "app/attr.h"
#include "app/config.h"
#include "seminline.h"
#include "term.h"
#include "scrollport.h"
#include "manager.h"
#include "thumbicon.h"
#include "mainwindow.h"

#include <QMimeData>

InlineSemantic::InlineSemantic(Tsqt::RegionType type, TermWidget *parent) :
    InlineBase(type, parent)
{
    m_renderer = ThumbIcon::getRenderer(ThumbIcon::SemanticType, m_icon);
}

InlineSemantic::InlineSemantic(TermWidget *parent) :
    InlineSemantic(Tsqt::RegionSemantic, parent)
{
}

void
InlineSemantic::setToolTip(const QString &link)
{
    QWidget::setToolTip(link.size() < SEMANTIC_TOOLTIP_MAX ?
                        link :
                        link.left(SEMANTIC_TOOLTIP_MAX) + C(0x2026));
}

void
InlineSemantic::setRegion(const Region *region)
{
    m_region = region;
    setUrl(TermUrl::parse(region->attributes.value(g_attr_CONTENT_URI)));
    setToolTip(region->attributes.value(g_attr_SEM_TOOLTIP));
    m_subrects.clear();

    QString icon = region->attributes.value(g_attr_SEM_ICON);
    if (m_icon != icon) {
        m_icon = icon;
        m_renderer = ThumbIcon::getRenderer(ThumbIcon::SemanticType, icon);
    }

    if (region->endRow == region->startRow) {
        setBounds(region->startCol, 0, region->endCol - region->startCol, 1);
    } else {
        int offset = 0;
        setBounds(region->startCol, 0, m_scrollport->width() - region->startCol, 1);

        for (auto i = region->endRow - region->startRow; i > 1; --i)
            m_subrects.append(QRect(0, ++offset, m_scrollport->width(), 1));

        if (region->endCol)
            m_subrects.append(QRect(0, offset + 1, region->endCol, 1));
    }
}

void
InlineSemantic::handleHighlight(regionid_t id)
{
    if (id == m_region->id() || id == INVALID_REGION_ID)
        setHighlighting();
}

void
InlineSemantic::open()
{
    setExecuting();

    // Note: this may reenter inline region processing!
    QString action = m_region->attributes.value(g_attr_SEM_ACTION1);
    if (!action.isEmpty())
        m_scrollport->manager()->invokeSlot(action);
}

QMenu *
InlineSemantic::getPopup(MainWindow *window)
{
    QString spec = m_region->attributes.value(g_attr_SEM_MENU);
    return window->getCustomPopup(m_term, spec, m_icon);
}

void
InlineSemantic::getDragData(QMimeData *data, QDrag *)
{
    QString spec = m_region->attributes.value(g_attr_SEM_DRAG);
    QStringList speclist = spec.split('\0');
    while (speclist.size() >= 2) {
        QString mimetype = speclist.takeFirst();
        QByteArray content = speclist.takeFirst().toUtf8();
        data->setData(mimetype, content);
    }
}
