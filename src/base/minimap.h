// Copyright © 2018 TermySequence LLC
//
// SPDX-License-Identifier: GPL-2.0-only

#pragma once

#include "fontbase.h"

#include <QWidget>
#include <QVector>
#include <QHash>

class TermScrollport;
class TermInstance;
class TermBuffers;
class TermMinipin;
class Region;

class TermMinimap final: public QWidget, public FontBase
{
    Q_OBJECT

private:
    TermScrollport *m_scrollport;
    TermInstance *m_term;
    TermBuffers *m_buffers;

    QVector<TermMinipin*> m_pins;
    int m_hiddenThreshold = 0;
    int m_recentPrompts;

    bool m_visible = false;
    bool m_havePos = false;
    bool m_showPos;

    QRectF m_bounds;
    QString m_index;
    qreal m_radius;
    qreal m_line;
    QRect m_posRect;
    QPointF m_posTri[3];

    struct OtherRec {
        QRectF bounds;
        QString index;
        QMetaObject::Connection moc;
    };

    QHash<TermScrollport*,OtherRec> m_others;

    QColor m_fg, m_bg, m_blend;
    QFont m_font;

    QSize m_sizeHint;

    void calculateBounds(TermScrollport *scrollport, QRectF &bounds);
    void placePin(TermMinipin *pin);
    void handleRegion(const Region *r, int &index);

protected:
    void paintEvent(QPaintEvent *event);
    void resizeEvent(QResizeEvent *event);
    void mousePressEvent(QMouseEvent *event);
    void mouseMoveEvent(QMouseEvent *event);
    void showEvent(QShowEvent *event);
    void hideEvent(QHideEvent *event);

private slots:
    void handleRegions();
    void updateScroll();
    void updateOther();
    void updateFetchPos();
    void updateStacks();
    void handleSettingsChanged();

    void recolor(QRgb bg, QRgb fg);
    void refont(const QFont &font);

public:
    TermMinimap(TermScrollport *scrollport, QWidget *parent);

    QSize sizeHint() const;
    QSize minimumSizeHint() const;
};
