// Copyright © 2018 TermySequence LLC
//
// SPDX-License-Identifier: GPL-2.0-only

#include "common.h"
#include "app/config.h"
#include "app/logging.h"
#include "blank.h"
#include "manager.h"
#include "fontbase.h"
#include "settings/settings.h"
#include "settings/keymap.h"
#include "settings/profile.h"
#include "settings/termlayout.h"

#include <QLabel>
#include <QScrollBar>
#include <QtMath>

#define TR_TEXT1 TL("window-text", "No terminal is active")

TermBlank::TermBlank(TermManager *manager, QWidget *parent) :
    QWidget(parent),
    m_manager(manager),
    m_focused(false)
{
    m_label = new QLabel(TR_TEXT1, this);
    m_label->setAlignment(Qt::AlignCenter);

    m_scroll = new QScrollBar(Qt::Vertical, this);
    m_scroll->setVisible(false);

    setFocusPolicy(Qt::StrongFocus);

    calculateSizeHint(g_settings->startupProfile());
}

inline void
TermBlank::refocus()
{
    if (m_manager->active() && m_focused)
        m_manager->setActiveTerm(nullptr, nullptr);
}

void
TermBlank::calculateSizeHint(const ProfileSettings *profile)
{
    QSizeF cellSize = FontBase::getCellSize(profile->font());

    const TermLayout layout(profile->layout());
    QSize termSize = profile->termSize();
    int margin = 1 + cellSize.height() / MARGIN_INCREMENT;

    int w = 2 * margin + qCeil(termSize.width() * cellSize.width());
    int h = 2 * margin + qCeil(termSize.height() * cellSize.height());
    int i;

    if (layout.itemEnabled(i = LAYOUT_WIDGET_SCROLL)) {
        w += m_scroll->sizeHint().width() + layout.itemExtraWidth(i, margin);
    }
    if (layout.itemEnabled(i = LAYOUT_WIDGET_MARKS)) {
        w += qCeil(2 * cellSize.width()) + layout.itemExtraWidth(i, margin);
    }
    if (layout.itemEnabled(i = LAYOUT_WIDGET_MINIMAP)) {
        w += qCeil(2 * cellSize.width()) + layout.itemExtraWidth(i, margin);
    }
    if (layout.itemEnabled(i = LAYOUT_WIDGET_MODTIME)) {
        w += qCeil(7 * cellSize.width()) + layout.itemExtraWidth(i, margin);
    }

    m_sizeHint = QSize(w, h);
    qCDebug(lcLayout) << this << "sizeHint is" << m_sizeHint;
}

QSize
TermBlank::calculateTermSize(const ProfileSettings *profile) const
{
    QSizeF cellSize = FontBase::getCellSize(profile->font());

    const TermLayout layout(profile->layout());
    int margin = 1 + cellSize.height() / MARGIN_INCREMENT;

    int w = m_currentSize.width() - 2 * margin;
    int h = m_currentSize.height() - 2 * margin;
    int i;

    if (layout.itemEnabled(i = LAYOUT_WIDGET_SCROLL)) {
        w -= m_scroll->sizeHint().width() + layout.itemExtraWidth(i, margin);
    }
    if (layout.itemEnabled(i = LAYOUT_WIDGET_MARKS)) {
        w -= qCeil(2 * cellSize.width()) + layout.itemExtraWidth(i, margin);
    }
    if (layout.itemEnabled(i = LAYOUT_WIDGET_MINIMAP)) {
        w -= qCeil(2 * cellSize.width()) + layout.itemExtraWidth(i, margin);
    }
    if (layout.itemEnabled(i = LAYOUT_WIDGET_MODTIME)) {
        w -= qCeil(7 * cellSize.width()) + layout.itemExtraWidth(i, margin);
    }

    w = qFloor((qreal)w / cellSize.width());
    h = qFloor((qreal)h / cellSize.height());

    QSize result(w, h);
    qCDebug(lcLayout) << this << "calculated size" << result << "for profile" << profile->name();
    return result;
}

bool
TermBlank::event(QEvent *event)
{
    if (event->type() == QEvent::KeyPress) {
        QKeyEvent *ke = static_cast<QKeyEvent *>(event);
        g_settings->defaultKeymap()->translate(m_manager, ke, Tsq::DefaultTermFlags);
        return true;
    }

    return QWidget::event(event);
}

void
TermBlank::focusInEvent(QFocusEvent *event)
{
    m_focused = true;
    refocus();
    QWidget::focusInEvent(event);
}

void
TermBlank::focusOutEvent(QFocusEvent *event)
{
    m_focused = false;
    refocus();
    g_settings->defaultKeymap()->reset();
    QWidget::focusOutEvent(event);
}

void
TermBlank::resizeEvent(QResizeEvent *event)
{
    qCDebug(lcLayout) << this << "resize to" << size();
    m_label->setGeometry(rect());
}

QSize
TermBlank::sizeHint() const
{
    return m_sizeHint;
}
