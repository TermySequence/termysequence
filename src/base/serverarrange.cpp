// Copyright © 2018 TermySequence LLC
//
// SPDX-License-Identifier: GPL-2.0-only

#include "common.h"
#include "app/config.h"
#include "serverarrange.h"
#include "thumbicon.h"
#include "server.h"
#include "manager.h"
#include "termformat.h"
#include "mainwindow.h"
#include "barwidget.h"
#include "smartlabel.h"
#include "settings/global.h"

#include <QPainter>
#include <QMenu>
#include <QContextMenuEvent>
#include <QMimeData>

ServerArrange::ServerArrange(ServerInstance *server, TermManager *manager, BarWidget *parent) :
    ThumbBase(server, manager, parent),
    m_server(server),
    m_icon(new ThumbIcon(ThumbIcon::ServerType, this)),
    m_disconnect(new ThumbIcon(ThumbIcon::IndicatorType, this)),
    m_caption(new EllipsizingLabel(this)),
    m_count(new QLabel(this))
{
    TermFormat *format = server->format();

    m_disconnect->setIcon(L("disconnected"), ThumbIcon::IndicatorType);
    m_disconnect->raise();

    connect(server, &IdBase::iconsChanged, m_icon, &ThumbIcon::setIcons);
    connect(server, SIGNAL(connectionChanged()), SLOT(handleConnection()));
    connect(manager, &TermManager::serverHidden, this, &ServerArrange::handleTermsChanged);
    connect(manager, &TermManager::serverActivated, this, &ServerArrange::refocus);
    connect(format, &TermFormat::captionChanged, m_caption, &EllipsizingLabel::setText);
    connect(format, &TermFormat::captionFormatChanged, this, &ServerArrange::updateGeometry);
    connect(format, &TermFormat::termCaptionLinesChanged, this, &ServerArrange::updateGeometry);
    connect(format, &TermFormat::tooltipChanged, this, &QWidget::setToolTip);

    m_caption->setAlignment(Qt::AlignCenter);
    m_caption->setText(format->caption());
    m_count->setTextFormat(Qt::PlainText);
    m_count->setAlignment(Qt::AlignLeft);
    setToolTip(format->tooltip());

    handleTermsChanged(server, manager->totalCount(server), manager->hiddenCount(server));
    m_icon->setIcons(server->icons(), server->iconTypes());
    handleConnection();
}

void
ServerArrange::handleTermsChanged(ServerInstance *server, size_t total, size_t hidden)
{
    if (m_server == server) {
        QString text;

        if (!total || total - hidden)
            text = QString::number(total);
        if (hidden)
            text.append(L("(%2)").arg(hidden));
        if (total - hidden)
            text.append(':');

        m_count->setText(text);

        relayout();
        updateGeometry();
    }
}

void
ServerArrange::refocus()
{
    bool hover = m_hover || m_normalDrag;
    bool primary = m_manager->activeServer() == m_server;

    ColorName fgName = NormalFg;

    if (primary) {
        m_bgName = hover ? ActiveHoverBg : ActiveBg;
        fgName = ActiveFg;
    } else if (hover) {
        m_bgName = HoverBg;
    } else {
        m_bgName = NormalBg;
    }

    if (m_fgName != fgName) {
        m_fgName = fgName;
        QString ss = A("background:none;color:") + m_colors[fgName].name();
        m_caption->setStyleSheet(ss);
        m_count->setStyleSheet(ss);
    }

    update();
}

void
ServerArrange::calculateBounds()
{
    int w = width() - 2 * THUMB_MARGIN;
    int h = height() - 2 * THUMB_MARGIN;

    int hcap = m_cellSize.height() * m_server->format()->termCaptionLines();
    int hmin = (h - hcap) / 3;
    int hmax = (h - hcap) / 2;

    int hi = h - m_caption->sizeHint().height();

    if (hi > hmax)
        hi = hmax;
    if (hi < hmin)
        hi = hmin;
    if (hi < m_cellSize.height())
        hi = m_cellSize.height();

    m_captionBounds.setRect(THUMB_MARGIN,
                            hi + 2 * THUMB_MARGIN,
                            w,
                            h - hi - THUMB_MARGIN);

    w = width();
    h = height();

    m_iconBounds.setRect((w - hi) / 2,
                         THUMB_MARGIN,
                         hi,
                         hi);

    int wc = m_count->sizeHint().width();
    int hc = m_count->sizeHint().height();

    m_countBounds.setRect(w - wc - THUMB_MARGIN,
                          THUMB_MARGIN + (hi - hc) / 2,
                          wc,
                          hc);

    QPainterPath corner;
    if (m_parent->horizontal()) {
        corner.arcTo(THUMB_MARGIN / 2, 0.0, w - hi, w - hi, 90.0, 90.0);
        corner.lineTo(THUMB_MARGIN / 2, h);
        corner.lineTo(0, h);
    } else {
        corner.arcTo(0.0, THUMB_MARGIN / 2, w - hi, w - hi, 180.0, -90.0);
        corner.lineTo(w, THUMB_MARGIN / 2);
        corner.lineTo(w, 0);
    }
    corner.closeSubpath();
    m_path = QPainterPath();
    m_path.addRect(rect());
    m_path -= corner;
}

void
ServerArrange::relayout()
{
    calculateBounds();

    m_icon->setGeometry(m_iconBounds);
    m_disconnect->setGeometry(m_iconBounds);
    m_caption->setGeometry(m_captionBounds);
    m_count->setGeometry(m_countBounds);
}

void
ServerArrange::paintEvent(QPaintEvent *event)
{
    QPainter painter(this);

    if (m_bgName != NormalBg) {
        painter.setRenderHint(QPainter::Antialiasing, true);
        painter.fillPath(m_path, m_colors[m_bgName]);
    }

    ThumbBase::paintCommon(&painter);
}

void
ServerArrange::resizeEvent(QResizeEvent *event)
{
    ThumbBase::resizeEvent(event);
    relayout();
}

bool
ServerArrange::event(QEvent *event)
{
    switch (event->type()) {
    case QEvent::Polish:
        QWidget::event(event);
        calculateColors(true);
        calculateCellSize(font());
        refocus();
        return true;
    case QEvent::LayoutRequest:
        relayout();
        updateGeometry();
        return true;
    default:
        return QWidget::event(event);
    }
}

int
ServerArrange::heightForWidth(int w) const
{
    return 3 * THUMB_MARGIN + m_caption->sizeHint().height() + 3 * m_cellSize.height();
}

int
ServerArrange::widthForHeight(int h) const
{
    int hcap = m_cellSize.height() * m_server->format()->termCaptionLines();
    int hmax = (h - hcap) / 2;
    int w1 = 3 * THUMB_MARGIN + 2 * m_count->sizeHint().width() + hmax;
    int w2 = m_caption->sizeHint().width();

    return w1 > w2 ? w1 : w2;
}

void
ServerArrange::contextMenuEvent(QContextMenuEvent *event)
{
    QMenu *m = m_manager->parent()->getServerPopup(m_server);
    m->popup(event->globalPos());
    event->accept();
}

void
ServerArrange::mouseAction(int index) const
{
    QString tmp;

    // index: 0-3:action, -1:press
    switch (index) {
    default:
        m_manager->actionSwitchServer(m_targetIdStr, g_mtstr);
        return;
    case 0:
        tmp = g_global->serverAction0();
        break;
    case 1:
        tmp = g_global->serverAction1();
        break;
    case 2:
        tmp = g_global->serverAction2();
        break;
    case 3:
        tmp = g_global->serverAction3();
        break;
    }

    if (!tmp.isEmpty()) {
        tmp.replace(A("<serverId>"), m_targetIdStr);
        m_manager->invokeSlot(tmp);
    }
}

void
ServerArrange::enterEvent(QEvent *event)
{
    m_hover = true;
    refocus();
}

void
ServerArrange::leaveEvent(QEvent *event)
{
    m_hover = false;
    refocus();
}

void
ServerArrange::handleConnection()
{
    m_disconnect->setVisible(!m_server->conn());
}

void
ServerArrange::clearDrag()
{
    ThumbBase::clearDrag();
    refocus();
}

bool
ServerArrange::setNormalDrag(const QMimeData *data)
{
    m_normalDrag = true;
    m_acceptDrag = data->hasUrls();

    refocus();

    return ThumbBase::setNormalDrag(data);
}

void
ServerArrange::dropNormalDrag(const QMimeData *data)
{
    if (data->hasUrls()) {
        m_manager->serverFileDrop(m_server, data);
    }
}
