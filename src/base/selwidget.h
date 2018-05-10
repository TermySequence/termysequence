// Copyright © 2018 TermySequence LLC
//
// SPDX-License-Identifier: GPL-2.0-only

#pragma once

#include "lib/types.h"

#include <QWidget>
#include <QVector>
#include <QRectF>

QT_BEGIN_NAMESPACE
class QPropertyAnimation;
QT_END_NAMESPACE
class TermWidget;
class TermScrollport;
class TermBuffers;
class TermPalette;
class Selection;
class SelectionHandle;

//
// Main widget
//
class SelectionWidget final: public QWidget
{
    Q_OBJECT

    friend class SelectionHandle;

private:
    TermWidget *m_parent;
    TermScrollport *m_scrollport;
    TermBuffers *m_buffers;
    Selection *m_sel;
    SelectionHandle *m_upper, *m_lower;

    QMetaObject::Connection m_mocOffset, m_mocHandle;

    QVector<QPointF> m_lines;
    qreal m_cw, m_ch, m_half;
    int m_line;

    bool m_active = false;
    char m_activeHandle = -1;
    QColor m_handleColors[2];

    QVector<qreal> m_dashPattern;
    int m_dashOffset = 0, m_dashSize, m_dashCount;
    int m_dashId = 0, m_dashTime;

    void activateHandle(bool upper);
    void activateHandleArg(int arg, bool defval);
    void switchHandleArg(int arg);

private slots:
    void handleStart();
    void handleRestart();
    void handleStop();
    void handleModify();

    void handleOffsetChanged();
    void handleSelectRequest(int type, int arg);

    void recolor();

protected:
    void paintEvent(QPaintEvent *event);
    void timerEvent(QTimerEvent *event);

public:
    SelectionWidget(TermScrollport *scrollport, TermWidget *parent);

    inline bool active() const { return m_active; }

    void startAnimation();
    void stopAnimation();

    void calculateDashParams(unsigned blinkTime);
    void calculateCellSize();
    void calculatePosition();

    static void initialize();
};

//
// Handle
//
class SelectionHandle final: public QWidget
{
    Q_OBJECT
    Q_PROPERTY(int bounce READ bounce WRITE setBounce)

private:
    SelectionWidget *m_parent;
    bool m_lower;
    bool m_active = false;
    bool m_selecting = false;

    QColor m_color;

    int m_realX;
    int m_bounce = 0;
    QPropertyAnimation *m_bounceAnim;

protected:
    void paintEvent(QPaintEvent *event);
    void mousePressEvent(QMouseEvent *event);
    void mouseReleaseEvent(QMouseEvent *event);
    void mouseMoveEvent(QMouseEvent *event);

public:
    SelectionHandle(bool lower, SelectionWidget *parent);

    void setActive(bool active);
    void move(int x, int y);

    inline int bounce() const { return m_bounce; }
    void setBounce(int bounce);
    void startBounce();

    static void paintPreview(QPainter *painter, const TermPalette *palette,
                             const QSizeF &cellSize, bool active);
};
