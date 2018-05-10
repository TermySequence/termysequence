// Copyright © 2018 TermySequence LLC
//
// SPDX-License-Identifier: GPL-2.0-only

#pragma once

#include "fontbase.h"
#include "blinktimer.h"

#include <QStyledItemDelegate>

namespace Tsq { class Unicoding; }

//
// Regular delegate
//
class FileViewItem final: public QStyledItemDelegate, public DisplayIterator
{
private:
    unsigned m_displayMask;

    void paintComplex(QPainter *painter, DisplayCell &dc, Tsq::Unicoding *unicoding) const;

public:
    FileViewItem(unsigned displayMask, QWidget *parent);

    void paint(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const;
};

//
// Filename delegate
//
class FileNameItem final: public QStyledItemDelegate, public DisplayIterator, public BlinkBase
{
private:
    QString m_arrow;
    TermBlinkTimer *m_blink;
    bool m_active = false;
    bool m_blinkSeen = false;

public:
    FileNameItem(QWidget *parent);

    void paint(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const;
    QSize sizeHint(const QStyleOptionViewItem &option, const QModelIndex &index) const;

public:
    void drawName(QPainter *painter, const QRect &rect, QStyle::State flags,
                  const QModelIndex &index) const;

public:
    inline void setTerm(TermInstance *term) { m_term = term; }

    inline bool active() const { return m_active; }
    inline void setActive(bool active) { m_active = active; }

    inline void setBlinkSeen(bool blinkSeen) {
        if (blinkSeen && !m_blinkSeen)
            m_blinkSeen = true;
    }
    inline void setBlinkEffect() {
        if (m_blinkSeen)
            m_blink->setBlinkEffect(this);
    }
};
