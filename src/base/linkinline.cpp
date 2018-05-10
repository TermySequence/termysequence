// Copyright © 2018 TermySequence LLC
//
// SPDX-License-Identifier: GPL-2.0-only

#include "common.h"
#include "app/attr.h"
#include "app/messagebox.h"
#include "linkinline.h"
#include "termwidget.h"
#include "term.h"
#include "server.h"
#include "mainwindow.h"
#include "semflash.h"
#include "settings/global.h"
#include "settings/servinfo.h"

#include <QMimeData>
#include <QDesktopServices>
#include <QSysInfo>

#define TR_ERROR1 TL("error", "The file's host '%1' does not match the local host '%2'")

InlineLink::InlineLink(TermWidget *parent) :
    InlineSemantic(Tsqt::RegionLink, parent)
{
}

void
InlineLink::setRegion(const Region *region)
{
    InlineSemantic::setRegion(region);
    m_isfile = m_url.scheme() == A("file");

    if (region->attributes.value(g_attr_CONTENT_TYPE) == A("515")) {
        switch (m_term->server()->serverInfo()->allowLinks()) {
        case -1:
            m_issem = g_global->allowLinks();
            break;
        case 1:
            m_issem = true;
            break;
        default:
            m_issem = false;
            break;
        }
    }
    else {
        m_issem = region->flags & Tsqt::Semantic;
    }

    if (!m_issem)
        setToolTip(m_url.toString());
    if (region->flash)
        connect(region->flash, &SemanticFlash::startFlash, this, &InlineBase::setHighlighting);
}

void
InlineLink::handleHighlight(regionid_t id)
{
    InlineBase::handleHighlight(id);
}

void
InlineLink::open()
{
    if (m_issem) {
        InlineSemantic::open();
    }
    else if (!m_isfile || m_url.checkHost()) {
        setExecuting();
        QDesktopServices::openUrl(m_url);
    }
    else {
        QString h = QSysInfo::machineHostName();
        errBox(TR_ERROR1.arg(m_url.host(), h), m_parent)->show();
    }
}

QMenu *
InlineLink::getPopup(MainWindow *window)
{
    if (m_issem) {
        return InlineSemantic::getPopup(window);
    }
    else if (m_isfile) {
        QString spec = QString('5') + C(0) + m_url.toString() + C(0) + C(0) + C(0);
        return window->getCustomPopup(m_term, spec, g_mtstr);
    }
    else {
        return window->getLinkPopup(m_term, m_url.toString());
    }
}

void
InlineLink::getDragData(QMimeData *data, QDrag *drag)
{
    if (m_issem) {
        InlineSemantic::getDragData(data, drag);
    } else {
        data->setUrls(QList<QUrl>({ m_url }));
        data->setText(m_url.toString());
    }
}
