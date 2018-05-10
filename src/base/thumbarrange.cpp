// Copyright © 2018 TermySequence LLC
//
// SPDX-License-Identifier: GPL-2.0-only

#include "common.h"
#include "app/config.h"
#include "thumbarrange.h"
#include "thumbframe.h"
#include "thumbicon.h"
#include "termformat.h"
#include "server.h"
#include "term.h"
#include "manager.h"
#include "mainwindow.h"
#include "smartlabel.h"
#include "settings/global.h"
#include "settings/profile.h"

#include <QPainter>
#include <QMenu>
#include <QContextMenuEvent>
#include <QMimeData>

ThumbArrange::ThumbArrange(TermInstance *term, TermManager *manager, BarWidget *parent) :
    ThumbBase(term, manager, parent),
    m_term(term),
    m_thumb(new ThumbFrame(term, manager, this)),
    m_icon(new ThumbIcon(ThumbIcon::CommandType, this)),
    m_lock(new ThumbIcon(ThumbIcon::IndicatorType, this)),
    m_owner(new ThumbIcon(ThumbIcon::AvatarType, this)),
    m_caption(new EllipsizingLabel(this))
{
    TermFormat *format = term->format();

    connect(term, &IdBase::iconsChanged, m_icon, &ThumbIcon::setIcons);
    connect(term, &IdBase::indicatorsChanged, this, &ThumbArrange::handleIndicators);
    connect(term, SIGNAL(stacksChanged()), SLOT(refocus()));
    connect(term, SIGNAL(miscSettingsChanged()), SLOT(handleIconSettings()));
    connect(manager, &TermManager::activeChanged, this, &ThumbArrange::refocus);
    connect(manager, SIGNAL(termActivated(TermInstance*,TermScrollport*)), SLOT(refocus()));
    connect(format, &TermFormat::captionChanged, m_caption, &EllipsizingLabel::setText);
    connect(format, &TermFormat::captionFormatChanged, this, &ThumbArrange::handleFormatChanged);
    connect(format, &TermFormat::tooltipChanged, this, &QWidget::setToolTip);

    m_icon->setIcons(term->icons(), term->iconTypes());
    handleIconSettings();
    handleFormatChanged(format->captionFormat());
    m_caption->setAlignment(Qt::AlignCenter);
    m_caption->setText(format->caption());
    setToolTip(format->tooltip());
}

void
ThumbArrange::handleFormatChanged(const QString &format)
{
    m_haveCaption = !format.isEmpty();
    m_caption->setVisible(m_haveCaption);

    relayout();
    updateGeometry();
}

void
ThumbArrange::refocus()
{
    bool hover = m_hover || m_normalDrag;
    // bool primary = m_manager->active() && m_manager->activeTerm() == m_term;
    bool primary = m_manager->activeTerm() == m_term;
    bool active = m_term->stackCount() != 0;

    ColorName fgName = NormalFg;

    if (primary) {
        m_bgName = hover ? ActiveHoverBg : ActiveBg;
        fgName = ActiveFg;
    } else if (active) {
        m_bgName = hover ? InactiveHoverBg : InactiveBg;
        fgName = InactiveFg;
    } else if (hover) {
        m_bgName = HoverBg;
    } else {
        m_bgName = NormalBg;
    }

    if (m_fgName != fgName) {
        m_fgName = fgName;
        QString ss = A("background:none;color:") + m_colors[fgName].name();
        m_caption->setStyleSheet(ss);
    }

    update();
}

inline void
ThumbArrange::calculateBounds()
{
    int w = m_outerBounds.width();
    int h = m_outerBounds.height();

    int l = m_thumb->widthForHeight(h);
    if (l > w) {
        l = m_thumb->heightForWidth(w);
        m_thumbBounds.setRect(THUMB_MARGIN, THUMB_MARGIN + (h - l) / 2, w, l);
    } else {
        m_thumbBounds.setRect(THUMB_MARGIN + (w - l) / 2, THUMB_MARGIN, l, h);
    }

    QSize iconSize(1, 1);
    iconSize.scale(w * ICON_WSCALE, h * ICON_HSCALE, Qt::KeepAspectRatio);
    QSize indSize(1, 1);
    indSize.scale(w * INDICATOR_WSCALE, h * INDICATOR_HSCALE, Qt::KeepAspectRatio);

    w = iconSize.width();
    h = iconSize.height();
    m_iconBounds.setRect(ICON_OFF + m_outerBounds.right() - w,
                         ICON_OFF + m_outerBounds.bottom() - h,
                         w, h);

    h = indSize.height();
    m_lockBounds.setRect(width() - h, 0, h, h);
    m_ownerBounds.setRect(0, 0, h, h);
}

void
ThumbArrange::relayout()
{
    int th = m_haveCaption ? m_caption->sizeHint().height() : 0;
    int ts = m_haveCaption ? THUMB_SEP : 0;

    m_outerBounds.setRect(THUMB_MARGIN,
                          THUMB_MARGIN,
                          width() - (2 * THUMB_MARGIN),
                          height() - (2 * THUMB_MARGIN) - ts - th);

    m_captionBounds.setRect(THUMB_MARGIN,
                            THUMB_MARGIN + THUMB_SEP + m_outerBounds.height(),
                            m_outerBounds.width(),
                            th);

    calculateBounds();

    m_thumb->setGeometry(m_thumbBounds);
    m_icon->setGeometry(m_iconBounds);
    m_caption->setGeometry(m_captionBounds);
    m_lock->setGeometry(m_lockBounds);
    m_owner->setGeometry(m_ownerBounds);

    update();
}

bool
ThumbArrange::event(QEvent *event)
{
    switch (event->type()) {
    case QEvent::Polish:
        QWidget::event(event);
        calculateColors(false);
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

void
ThumbArrange::paintEvent(QPaintEvent *event)
{
    QPainter painter(this);

    if (m_bgName != NormalBg)
        painter.fillRect(rect(), m_colors[m_bgName]);

    ThumbBase::paintCommon(&painter);
}

void
ThumbArrange::resizeEvent(QResizeEvent *event)
{
    ThumbBase::resizeEvent(event);
    relayout();
}

int
ThumbArrange::heightForWidth(int w) const
{
    int th = m_haveCaption ? m_caption->sizeHint().height() : 0;

    w -= 2 * THUMB_MARGIN;

    return 2 * THUMB_MARGIN + THUMB_SEP + m_thumb->heightForWidth(w) + th;
}

int
ThumbArrange::widthForHeight(int h) const
{
    int th = m_haveCaption ? m_caption->sizeHint().height() : 0;

    h -= 2 * THUMB_MARGIN + THUMB_SEP + th;

    return 2 * THUMB_MARGIN + m_thumb->widthForHeight(h);
}

void
ThumbArrange::contextMenuEvent(QContextMenuEvent *event)
{
    QMenu *m = m_manager->parent()->getThumbPopup(this);
    m->popup(event->globalPos());
    event->accept();
}

void
ThumbArrange::mouseAction(int index) const
{
    QString tmp;

    switch (index) {
    default:
        m_manager->actionSwitchTerminal(m_targetIdStr, g_mtstr);
        return;
    case 0:
        tmp = g_global->termAction0();
        break;
    case 1:
        tmp = g_global->termAction1();
        break;
    case 2:
        tmp = g_global->termAction2();
        break;
    case 3:
        tmp = g_global->termAction3();
        break;
    }

    if (!tmp.isEmpty()) {
        tmp.replace(A("<terminalId>"), m_targetIdStr);
        tmp.replace(A("<serverId>"), m_term->server()->idStr());
        m_manager->invokeSlot(tmp);
    }
}

void
ThumbArrange::enterEvent(QEvent *event)
{
    m_hover = true;
    refocus();
}

void
ThumbArrange::leaveEvent(QEvent *event)
{
    m_hover = false;
    refocus();
}

void
ThumbArrange::handleIconSettings()
{
    m_showIcon = m_term->profile()->showIcon();
    m_icon->setVisible(m_showIcon && !m_normalDrag);

    m_showIndicators = m_term->profile()->thumbIndicator();
    handleIndicators();
}

void
ThumbArrange::handleIndicators()
{
    const QString *i = m_term->indicators();
    const int *t = m_term->indicatorTypes();

    bool showOwner = t[0] != ThumbIcon::InvalidType;
    bool showLock = t[1] != ThumbIcon::InvalidType;

    if (i[0] != L("disconnected")) {
        showOwner = showOwner && m_showIndicators;
        showLock = showLock && (i[1] == L("urgent") || m_showIndicators);
    } else {
        showOwner = true;
        showLock = false;
    }

    if (showOwner)
        m_owner->setIcon(i[0], (ThumbIcon::IconType)t[0]);
    m_owner->setVisible(showOwner);

    if (showLock)
        m_lock->setIcon(i[1], ThumbIcon::IndicatorType);
    m_lock->setVisible(showLock);
}

void
ThumbArrange::clearDrag()
{
    ThumbBase::clearDrag();
    m_icon->setVisible(m_showIcon);
    refocus();
}

bool
ThumbArrange::setNormalDrag(const QMimeData *data)
{
    m_normalDrag = true;
    m_acceptDrag = data->hasText() || data->hasUrls();

    m_icon->setVisible(false);
    refocus();

    return ThumbBase::setNormalDrag(data);
}

void
ThumbArrange::dropNormalDrag(const QMimeData *data)
{
    if (data->hasUrls()) {
        m_manager->thumbnailFileDrop(m_term, data);
    }
    else if (data->hasText()) {
        m_term->pushInput(m_manager, data->text());
    }
}

bool
ThumbArrange::hidden() const
{
    return m_manager->isHidden(m_term);
}
