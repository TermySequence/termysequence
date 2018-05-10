// Copyright © 2018 TermySequence LLC
//
// SPDX-License-Identifier: GPL-2.0-only

#pragma once

#include "app/enums.h"
#include "lib/types.h"

#include <QWidget>
#include <QRectF>

QT_BEGIN_NAMESPACE
class QSvgRenderer;
QT_END_NAMESPACE
class TermInstance;
class TermScrollport;
class Region;

class TermMinipin final: public QWidget
{
    Q_OBJECT

private:
    regionid_t m_regionId;
    Tsqt::RegionType m_regionType;

    index_t m_startRow, m_endRow;

    TermInstance *m_term;
    TermScrollport *m_scrollport;

    QColor m_fg, m_bg;
    QRect m_border;
    QRectF m_icon;
    QSvgRenderer *m_renderer;
    QString m_text;

protected:
    void paintEvent(QPaintEvent *event);
    void resizeEvent(QResizeEvent *event);
    void contextMenuEvent(QContextMenuEvent *event);
    void mousePressEvent(QMouseEvent *event);

public:
    TermMinipin(TermScrollport *scrollport, QWidget *parent);

    inline index_t startRow() const { return m_startRow; }
    inline index_t endRow() const { return m_endRow; }

    inline auto regionType() const { return m_regionType; }
    bool setRegion(const Region *region);
};
