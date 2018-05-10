// Copyright © 2018 TermySequence LLC
//
// SPDX-License-Identifier: GPL-2.0-only

#pragma once

#include "settings/termlayout.h"

#include <QWidget>
#include <QRectF>

QT_BEGIN_NAMESPACE
class QFile;
QT_END_NAMESPACE
class TermStack;
class TermInstance;
class TermScrollport;
class TermWidget;
class TermModtimes;

#define ARRANGE_N_WIDGETS (2 * LAYOUT_N_WIDGETS)

//
// Main widget
//
class TermArrange final: public QWidget
{
    Q_OBJECT

private:
    TermLayout m_layout;

    TermInstance *m_term;
    TermScrollport *m_scrollport;
    TermWidget *m_widget;
    TermModtimes *m_modtimes;

    QWidget *m_widgets[ARRANGE_N_WIDGETS]{};

    QRectF m_focusFadeBounds;
    int m_margin;

    QSize m_sizeHint;

    void calculateSizeHint();
    void calculateBounds(int w, int h, QRect *rects);

private slots:
    void rebackground(QRgb bg);
    void handleLayoutChanged(const QString &layoutStr);

protected:
    bool event(QEvent *event);
    void paintEvent(QPaintEvent *event);
    void resizeEvent(QResizeEvent *event);

public:
    TermArrange(TermStack *stack, TermInstance *term);

    inline TermScrollport* scrollport() { return m_scrollport; }
    inline TermModtimes* modtimes() { return m_modtimes; }
    inline TermWidget* widget() { return m_widget; }

    void relayout(const QSize &size);

    void copyImage();
    bool saveAsImage(QFile *file);

    QSize sizeHint() const;
};

//
// Separator widget
//
class TermSeparator final: public QWidget
{
    Q_OBJECT

private:
    QColor m_color;

private slots:
    void rebackground(QRgb bg, QRgb fg);

protected:
    void paintEvent(QPaintEvent *event);

public:
    TermSeparator(TermInstance *term, QWidget *parent);
};
