// Copyright © 2018 TermySequence LLC
//
// SPDX-License-Identifier: GPL-2.0-only

#include "common.h"
#include "app/actions.h"
#include "app/color.h"
#include "app/icons.h"
#include "statuslabel.h"
#include "manager.h"
#include "infoanim.h"
#include "menubase.h"
#include "settings/global.h"
#include "settings/keymap.h"

#include <QPainter>
#include <QDesktopServices>
#include <QMouseEvent>

#define TR_KEYMAP1 TL("keymap", "unbound")

//
// Base class
//
StatusLabel::StatusLabel()
{
    setTextFormat(Qt::RichText);
    setContextMenuPolicy(Qt::NoContextMenu);
}

StatusLabel::StatusLabel(const QString &text) :
    QLabel(text)
{
    setTextFormat(Qt::RichText);
    setContextMenuPolicy(Qt::NoContextMenu);
}

void
StatusLabel::setUrl(const QString &url)
{
    m_url = url;
    setCursor(Qt::PointingHandCursor);
}

void
StatusLabel::mousePressEvent(QMouseEvent *event)
{
    if (event->button() == Qt::LeftButton) {
        if (m_url.isEmpty()) {
            emit clicked();
        } else {
            QDesktopServices::openUrl(m_url);
        }

        event->accept();
    }
}

void
StatusLabel::enterEvent(QEvent *)
{
    m_hover = true;
    update();
}

void
StatusLabel::leaveEvent(QEvent *)
{
    m_hover = false;
    update();
}

void
StatusLabel::paintEvent(QPaintEvent *event)
{
    if (m_hover) {
        QPainter painter(this);
        painter.fillRect(rect(), m_bg);
    }

    QLabel::paintEvent(event);
}

bool
StatusLabel::event(QEvent *event)
{
    bool rc = QLabel::event(event);

    if (event->type() == QEvent::Polish) {
        m_bg = Colors::whiten(palette().color(QPalette::Highlight));
        m_bg.setAlphaF(0.4);
    }

    return rc;
}

//
// Action class
//
ActionLabel::ActionLabel(TermManager *manager, Tsq::TermFlags flags) :
    m_manager(manager),
    m_flags(flags|Tsq::DefaultTermFlags)
{
    setContextMenuPolicy(Qt::DefaultContextMenu);
    connect(this, SIGNAL(linkActivated(const QString&)), SLOT(handleLinkActivated()));
}

ActionLabel::ActionLabel(TermManager *manager, Tsq::TermFlags flags,
                         const QString &slot, const QString &text) :
    ActionLabel(manager, flags)
{
    m_slot = slot;
    m_text = text;
    setToolTip(actionString(slot));
}

void
ActionLabel::setAction(const QString &slot, const QString &text)
{
    m_slot = slot;
    setText(text.arg(slot));
    setToolTip(actionString(slot));

    if (m_animation && m_animation->state() != QAbstractAnimation::Stopped) {
        m_animation->stop();
        m_fade = 0;
        update();
    }
}

bool
ActionLabel::updateShortcut(const TermKeymap *keymap, TermShortcut *result)
{
    if (keymap->lookupShortcut(m_slot, m_flags, result)) {
        setText(m_text.arg(result->expression));
        return true;
    } else {
        setText(m_text.arg(TR_KEYMAP1));
        return false;
    }
}

void
ActionLabel::setFlags(const TermKeymap *keymap, Tsq::TermFlags flags)
{
    m_flags = flags | Tsq::DefaultTermFlags;
    TermShortcut tmp;
    updateShortcut(keymap, &tmp);
}

void
ActionLabel::mousePressEvent(QMouseEvent *event)
{
    // Avoid base class method
    QLabel::mousePressEvent(event);
}

void
ActionLabel::handleLinkActivated()
{
    emit invokingSlot(true);
    m_manager->invokeSlot(m_slot);
    emit invokingSlot(false);
}

void
ActionLabel::startAnimation()
{
    if (!m_animation) {
        m_animation = new InfoAnimation(this);
        connect(m_animation, SIGNAL(animationSignal(intptr_t)), SLOT(handleAnimation()));
    }
    m_animation->startColor(this);
}

void
ActionLabel::handleAnimation()
{
    m_fade = m_animation->fade();
    update();
}

void
ActionLabel::paintEvent(QPaintEvent *event)
{
    StatusLabel::paintEvent(event);

    if (m_fade) {
        QPainter painter(this);
        painter.fillRect(rect(), m_animation->color());
    }
}

void
ActionLabel::contextMenuEvent(QContextMenuEvent *event)
{
    if (!m_slot.isEmpty()) {
        QString action = m_slot.left(m_slot.indexOf('|'));
        QString url = g_global->docUrl(A("actions.html#") + action);

        auto *m = new DynamicMenu("pmenu-binding", m_manager->parent(), this);
        connect(m, SIGNAL(aboutToHide()), m, SLOT(deleteLater()));

        m->addAction(TL("pmenu-binding", "&Invoke \"%1\"").arg(m_slot),
                     toolTip(), QI(ICON_EXECUTE), m_slot);
        m->addAction(TL("pmenu-binding", "&Documentation for \"%1\"").arg(action),
                     ACT_HELP_BINDING.arg(action), QI(ICON_HELP_CONTENTS),
                     A("OpenDesktopUrl|") + url);

        m->popup(event->globalPos());
    }
    event->accept();
}
