// Copyright © 2018 TermySequence LLC
//
// SPDX-License-Identifier: GPL-2.0-only

#pragma once

#include "fontbase.h"

#include <QWidget>
#include <QVector>

class TermScrollport;
class TermInstance;
class TermMark;

class TermMarks final: public QWidget, public FontBase
{
    Q_OBJECT

private:
    TermScrollport *m_scrollport;
    TermInstance *m_term;

    QVector<TermMark*> m_marks;
    int m_hiddenThreshold = 0;

    bool m_visible = false;

    QSize m_markSize;
    QSize m_sizeHint;

private slots:
    void handleRegions();
    void refont(const QFont &font);

protected:
    void contextMenuEvent(QContextMenuEvent *event);
    void mousePressEvent(QMouseEvent *event);
    void showEvent(QShowEvent *event);
    void hideEvent(QHideEvent *event);

public:
    TermMarks(TermScrollport *scrollport, QWidget *parent);

    inline TermInstance* term() { return m_term; }

    QSize sizeHint() const;
    QSize minimumSizeHint() const;
};
