// Copyright © 2018 TermySequence LLC
//
// SPDX-License-Identifier: GPL-2.0-only

#pragma once

#include "cell.h"

#include <QFontMetricsF>
#include <QFontInfo>
#include <QPainter>

class TermViewport;
class TermInstance;

typedef std::vector<DisplayCell> DisplayCellList;

//
// Font calculations
//
class FontBase
{
protected:
    FontBase();

    QSizeF m_cellSize;
    QFontMetricsF m_metrics;
    qreal m_ascent;

    void calculateCellSize(const QFont &font);
    void calculateCellSize(const QString &fontStr);

public:
    inline QSizeF cellSize() const { return m_cellSize; }

    static QSizeF getCellSize(const QFont &font);
    static QString getDefaultFont();

    static inline bool isFixedWidth(const QFont &font)
    { return QFontInfo(font).fixedPitch(); }
};

//
// Display of text data
//
class DisplayIterator: public FontBase
{
protected:
    TermInstance *m_term;
    mutable DisplayCellList m_displayCells;
    DisplayCell m_cursorCell;
    DisplayCell m_mouseCell;

    QFont m_font;
    int m_fontWeight;
    // bool m_fontUnderline;

private:
    void recomposeCell(DisplayCell &dc, unsigned clusters,
                       qreal textwidth, qreal charwidth);
    void emojifyCell(DisplayCell &dc);

    void paintOverride(QPainter &painter, const DisplayCell &i, QRgb bgval) const;

    bool decomposeStringCells(DisplayCell &dc, size_t endptr);

protected:
    DisplayIterator();
    DisplayIterator(TermInstance *term);

    void calculateCells(TermViewport *viewport, bool doMouse);

    void paintSimple(QPainter &painter, const DisplayCell &i) const;
    void paintCell(QPainter &painter, const DisplayCell &i, CellState &state) const;
    void paintTerm(QPainter &painter, const DisplayCell &i, CellState &state) const;
    void paintThumb(QPainter &painter, const DisplayCell &i, CellState &state) const;

    qreal decomposeStringPixels(DisplayCell &dc) const;
    unsigned decomposeStringCells(DisplayCell &dc, unsigned maxCells, int *state);

    void elideCellsLeft(qreal actualWidth, qreal targetWidth);

public:
    void setDisplayFont(const QFont &font);

    qreal stringPixelWidth(const QString &str) const;
    column_t stringCellWidth(const QString &str) const;
};
