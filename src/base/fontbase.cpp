// Copyright © 2018 TermySequence LLC
//
// SPDX-License-Identifier: GPL-2.0-only

#include "common.h"
#include "app/attr.h"
#include "app/logging.h"
#include "fontbase.h"
#include "viewport.h"
#include "term.h"
#include "buffers.h"
#include "thumbicon.h"
#include "u2500.h"
#include "lib/grapheme.h"

#include <QFontDatabase>
#include <QSvgRenderer>
#include <QtMath>

#define REPS 100
static const QString s_refChar = L("n").repeated(REPS);

//
// Font calculations
//
FontBase::FontBase() : m_metrics(QFont())
{
}

QSizeF
FontBase::getCellSize(const QFont &font)
{
    const QFontMetricsF metrics(font);
    QSizeF result;

    /* Need to handle fixed-width fonts that have *fractional* width */
    result.setWidth(metrics.width(s_refChar) / REPS);
    result.setHeight(metrics.height());

    if (result.width() == 0)
        result.setWidth(10);
    if (result.height() == 0)
        result.setHeight(20);

    return result;
}

void
FontBase::calculateCellSize(const QFont &font)
{
    m_metrics = QFontMetricsF(font);

    /* Need to handle fixed-width fonts that have *fractional* width */
    m_cellSize.setWidth(m_metrics.width(s_refChar) / REPS);
    m_cellSize.setHeight(m_metrics.height());
    m_ascent = m_metrics.ascent();

    if (m_cellSize.width() == 0)
        m_cellSize.setWidth(10);
    if (m_cellSize.height() == 0)
        m_cellSize.setHeight(20);
}

void
FontBase::calculateCellSize(const QString &fontStr)
{
    QFont font;
    font.fromString(fontStr);
    calculateCellSize(font);
}

QString
FontBase::getDefaultFont()
{
    QFont font(L("Monospace"));
    if (isFixedWidth(font))
        goto out;

    font = QFontDatabase::systemFont(QFontDatabase::FixedFont);
out:
    qCDebug(lcSettings) << "Default font is" << font.toString();
    return font.toString();
}

//
// Display of text data
//
DisplayIterator::DisplayIterator() : m_term(nullptr)
{
}

DisplayIterator::DisplayIterator(TermInstance *term) : m_term(term)
{
}

void
DisplayIterator::setDisplayFont(const QFont &font)
{
    calculateCellSize(m_font = font);
    m_fontWeight = font.weight();
}

void
DisplayIterator::emojifyCell(DisplayCell &dc)
{
    qreal w = m_cellSize.width();
    if (dc.flags & Tsq::DblWidthChar)
        w *= 2;

    dc.rect.setWidth(w);

    Tsq::GraphemeWalk tbf(m_term->unicoding(), dc.substr);

    while (tbf.next()) {
        dc.text = QString::fromStdString(tbf.getEmojiName());
        m_displayCells.emplace_back(dc);
        dc.rect.translate(w, 0.0);
        dc.point.rx() += w;
    }
}

void
DisplayIterator::recomposeCell(DisplayCell &dc, unsigned clusters,
                               qreal textwidth, qreal charwidth)
{
    if (clusters == 1) {
        if (dc.flags & Tsq::DblWidthChar || textwidth > charwidth)
            dc.point.rx() += (dc.rect.width() - textwidth) / 2.0;
        m_displayCells.emplace_back(std::move(dc));
        return;
    }

    Tsq::GraphemeWalk tbf(m_term->unicoding(), dc.substr);
    unsigned splitpos = clusters / 2;
    clusters -= splitpos;

    for (unsigned i = 0; i < splitpos; ++i)
        tbf.next();

    if (!tbf.finished())
    {
        DisplayCell first(dc);
        first.substr = dc.substr.substr(0, tbf.end());
        first.text = QString::fromStdString(first.substr);
        qreal w = charwidth * splitpos;
        first.rect.setWidth(w);
        qreal t = m_metrics.width(first.text);

        if (qFabs(w - t) < 1.0)
            m_displayCells.emplace_back(std::move(first));
        else
            recomposeCell(first, splitpos, t, charwidth);

        dc.substr = dc.substr.substr(tbf.end());
        dc.text = QString::fromStdString(dc.substr);
        dc.rect.translate(w, 0.0);
        dc.point.rx() += w;
        w = charwidth * clusters;
        dc.rect.setWidth(w);
        t = m_metrics.width(dc.text);

        if (qFabs(w - t) < 1.0)
            m_displayCells.emplace_back(std::move(dc));
        else
            recomposeCell(dc, clusters, t, charwidth);
    }
}

void
DisplayIterator::calculateCells(TermViewport *viewport, bool doMouse)
{
    m_displayCells.clear();

    size_t cur = viewport->m_offset;
    size_t r = viewport->m_buffers->size() - cur;
    size_t h = viewport->m_bounds.height();
    size_t end = cur + (r < h ? r : h);
    qreal x, y, w, t;

    // Content
    for (y = 0.0; cur < end; ++cur)
    {
        const CellRow &row = viewport->m_buffers->row(cur);
        for (const auto &cell: row.cells)
        {
            x = cell.cellx * m_cellSize.width();
            w = cell.cellwidth * m_cellSize.width();

            DisplayCell dc(cell);
            dc.substr = row.str.substr(cell.startptr, cell.endptr - cell.startptr);
            dc.rect.setRect(x, y, w, m_cellSize.height());
            dc.lineFlags = row.flags & Tsq::DblLineMask;

            if (dc.flags & Tsq::EmojiChar) {
                emojifyCell(dc);
                continue;
            }

            dc.point = QPointF(x, y + m_ascent);
            dc.text = QString::fromStdString(dc.substr);
            t = m_metrics.width(dc.text);

            if (qFabs(w - t) < 1.0) {
                m_displayCells.emplace_back(std::move(dc));
            } else {
                w = m_cellSize.width();
                unsigned clusters = cell.cellwidth;

                if (dc.flags & Tsq::DblWidthChar) {
                    w *= 2;
                    clusters /= 2;
                }
                if (dc.flags & Tsq::Underline) {
                    // Handle the underline with a separate cell of spaces
                    DisplayCell ul(dc);
                    ul.flags &= ~Tsqt::VisibleBg;
                    ul.text = L(" ").repeated(cell.cellwidth);
                    m_displayCells.emplace_back(std::move(ul));
                    dc.flags &= ~(Tsqt::CellFlags)Tsq::Underline;
                }

                recomposeCell(dc, clusters, t, w);
            }
        }

        y += m_cellSize.height();
    }

    // Cursor
    QPoint cursor = viewport->cursor();
    cur = viewport->m_offset + cursor.y();
    m_cursorCell = DisplayCell();

    if (cur < end) {
        const CellRow &row = viewport->m_buffers->row(cur);
        Cell cell = viewport->m_buffers->cellByPos(cur, cursor.x());

        m_cursorCell.CellAttributes::operator=(cell);
        m_cursorCell.substr = row.str.substr(cell.startptr, cell.endptr - cell.startptr);
        m_cursorCell.lineFlags = row.flags & Tsq::DblLineMask;

        x = cell.cellx * m_cellSize.width();
        y = cursor.y() * m_cellSize.height();
        w = cell.cellwidth * m_cellSize.width();
        m_cursorCell.rect.setRect(x, y, w, m_cellSize.height());

        m_cursorCell.text = QString::fromStdString(m_cursorCell.substr);
        w -= m_metrics.width(m_cursorCell.text);
        m_cursorCell.point = QPointF(x + w / 2.0, y + m_ascent);

        if (cell.flags & Tsq::EmojiChar) {
            Tsq::GraphemeWalk tbf(m_term->unicoding(), m_cursorCell.substr);
            tbf.next();
            m_cursorCell.text = QString::fromStdString(tbf.getEmojiName());
        }
    }

    // Mouse
    if (doMouse) {
        cursor = viewport->mousePos();
        m_mouseCell = DisplayCell();

        if (cursor.x() >= 0) {
            cur = viewport->m_offset + cursor.y();
            Cell cell = viewport->m_buffers->cellByX(cur, cursor.x());

            m_mouseCell.CellAttributes::operator=(cell);
            x = cursor.x() * m_cellSize.width();
            y = cursor.y() * m_cellSize.height();
            w = m_cellSize.width();
            m_mouseCell.rect.setRect(x, y, w, m_cellSize.height());
        }
    }
}

static inline void
paintEmoji(QPainter &painter, const DisplayCell &i)
{
    ThumbIcon::getRenderer(ThumbIcon::EmojiType, i.text)->render(&painter, i.rect);
}

void
DisplayIterator::paintSimple(QPainter &painter, const DisplayCell &i) const
{
    // Colors
    painter.setPen(QColor(i.fg));

    // Font
    if (i.flags & Tsq::Bold) {
        QFont font = painter.font();
        font.setWeight((i.flags & Tsq::Bold) ? QFont::Bold : m_fontWeight);
        painter.setFont(font);
    }

    // Normal text
    if (i.flags & Tsq::EmojiChar) {
        paintEmoji(painter, i);
    } else {
        painter.drawText(i.point, i.text);
    }
}

void
DisplayIterator::paintCell(QPainter &painter, const DisplayCell &i, CellState &state) const
{
    QRgb fgval, bgval;

    // Colors
    if (i.flags & Tsq::Fg) {
        fgval = (i.flags & Tsq::FgIndex) ? m_term->palette().std(i.fg) : i.fg;
    } else {
        fgval = state.fg;
    }

    if (i.flags & Tsq::Bg) {
        bgval = (i.flags & Tsq::BgIndex) ? m_term->palette().std(i.bg) : i.bg;
    } else {
        bgval = state.bg;
    }

    if (i.flags & Tsq::Inverse) {
        QRgb tmp = bgval;
        bgval = fgval;
        fgval = tmp;
    }

    painter.setPen(QColor(fgval));

    // Font
    if ((i.flags & Tsq::Bold) ^ (state.flags & Tsq::Bold)) {
        state.flags &= ~Tsq::Bold;
        state.flags |= (i.flags & Tsq::Bold);
        QFont font = painter.font();
        font.setWeight((i.flags & Tsq::Bold) ? QFont::Bold : m_fontWeight);
        painter.setFont(font);
    }
    if ((i.flags & Tsq::Underline) ^ (state.flags & Tsq::Underline)) {
        state.flags &= ~Tsq::Underline;
        state.flags |= (i.flags & Tsq::Underline);
        QFont font = painter.font();
        font.setUnderline(i.flags & Tsq::Underline);
        painter.setFont(font);
    }

    // Normal text
    if (i.flags & Tsqt::ThumbFill)
        painter.fillRect(i.rect, QColor(bgval));

    if (!(i.flags & state.invisibleFlags)) {
        if (i.flags & Tsq::EmojiChar) {
            paintEmoji(painter, i);
        } else {
            painter.drawText(i.point, i.text);
        }
    }
}

void
DisplayIterator::paintOverride(QPainter &painter, const DisplayCell &i, QRgb bgval) const
{
    if (i.flags & Tsqt::PaintU2500) {
        drawing::paintU2500(painter, i, m_cellSize, bgval);
    } else {
        paintEmoji(painter, i);
    }
}

void
DisplayIterator::paintTerm(QPainter &painter, const DisplayCell &i, CellState &state) const
{
    QRgb fgval, bgval;

    // Colors
    if (i.flags & Tsq::Fg) {
        fgval = (i.flags & Tsq::FgIndex) ? m_term->palette().std(i.fg) : i.fg;
    } else {
        fgval = state.fg;
    }

    if (i.flags & Tsq::Bg) {
        bgval = (i.flags & Tsq::BgIndex) ? m_term->palette().std(i.bg) : i.bg;
    } else {
        bgval = state.bg;
    }

    if (i.flags & Tsq::Inverse) {
        QRgb tmp = bgval;
        bgval = fgval;
        fgval = tmp;
    }
    if (i.flags & Tsqt::Special) {
        fgval = m_term->palette().specialFg(i.flags, fgval);
        bgval = m_term->palette().specialBg(i.flags, bgval);
    }
    if (!(i.flags & Tsqt::Selected) != !(i.flags & Tsqt::SolidCursor)) {
        QRgb tmp = bgval;
        bgval = fgval;
        fgval = tmp;
    }

    painter.setPen(QColor(fgval));

    // Font
    if ((i.flags & Tsq::Bold) ^ (state.flags & Tsq::Bold)) {
        state.flags &= ~Tsq::Bold;
        state.flags |= (i.flags & Tsq::Bold);
        QFont font = painter.font();
        font.setWeight((i.flags & Tsq::Bold) ? QFont::Bold : m_fontWeight);
        painter.setFont(font);
    }
    if ((i.flags & Tsq::Underline) ^ (state.flags & Tsq::Underline)) {
        state.flags &= ~Tsq::Underline;
        state.flags |= (i.flags & Tsq::Underline);
        QFont font = painter.font();
        font.setUnderline(i.flags & Tsq::Underline);
        painter.setFont(font);
    }

    // Normal text
    if (i.lineFlags == 0) {
        if (i.flags & Tsqt::TermFill)
            painter.fillRect(i.rect, QColor(bgval));

        if (!(i.flags & state.invisibleFlags)) {
            if (i.flags & Tsqt::PaintOverride) {
                paintOverride(painter, i, bgval);
            } else {
                painter.drawText(i.point, i.text);
            }
        }

        return;
    }

    // Doublesize text
    qreal x = i.rect.x();
    qreal y = i.rect.y();

    if (i.lineFlags & Tsq::DblTopLine) {
        painter.scale(2.0, 1.0);
        painter.setClipRect(i.rect);
        painter.translate(x, y);
        painter.scale(1.0, 2.0);
        painter.translate(-x, -y);
    }
    else if (i.lineFlags & Tsq::DblBottomLine) {
        painter.scale(2.0, 1.0);
        painter.setClipRect(i.rect);
        painter.translate(x, y - m_cellSize.height());
        painter.scale(1.0, 2.0);
        painter.translate(-x, -y);
    }
    else {
        painter.translate(0, y);
        painter.scale(2.0, 1.0);
        painter.translate(0, -y);
    }

    if (i.flags & Tsqt::TermFill)
        painter.fillRect(i.rect, QColor(bgval));

    if (!(i.flags & state.invisibleFlags)) {
        if (i.flags & Tsqt::PaintOverride) {
            paintOverride(painter, i, bgval);
        } else {
            painter.drawText(i.point, i.text);
        }
    }

    painter.setClipping(false);
    painter.resetTransform();
}

void
DisplayIterator::paintThumb(QPainter &painter, const DisplayCell &i, CellState &state) const
{
    QRgb fgval, bgval;

    // Colors
    if (i.flags & Tsq::Fg) {
        fgval = (i.flags & Tsq::FgIndex) ? m_term->palette().std(i.fg) : i.fg;
    } else if (i.flags & Tsqt::Special) {
        fgval = m_term->palette().specialFg(i.flags, state.fg);
    } else {
        fgval = state.fg;
    }

    if (i.flags & Tsq::Bg) {
        bgval = (i.flags & Tsq::BgIndex) ? m_term->palette().std(i.bg) : i.bg;
    } else if (i.flags & Tsqt::Special) {
        bgval = m_term->palette().specialBg(i.flags, state.bg);
    } else {
        bgval = state.bg;
    }

    if (i.flags & Tsq::Inverse) {
        QRgb tmp = bgval;
        bgval = fgval;
        fgval = tmp;
    }

    painter.setPen(QColor(fgval));

    // Font
    if ((i.flags & Tsq::Bold) ^ (state.flags & Tsq::Bold)) {
        state.flags &= ~Tsq::Bold;
        state.flags |= (i.flags & Tsq::Bold);
        QFont font = painter.font();
        font.setWeight((i.flags & Tsq::Bold) ? QFont::Bold : m_fontWeight);
        painter.setFont(font);
    }
    if ((i.flags & Tsq::Underline) ^ (state.flags & Tsq::Underline)) {
        state.flags &= ~Tsq::Underline;
        state.flags |= (i.flags & Tsq::Underline);
        QFont font = painter.font();
        font.setUnderline(i.flags & Tsq::Underline);
        painter.setFont(font);
    }

    // Normal text
    if (i.lineFlags == 0) {
        if (i.flags & Tsqt::ThumbFill)
            painter.fillRect(i.rect, QColor(bgval));

        if (!(i.flags & state.invisibleFlags)) {
            if (i.flags & Tsqt::PaintOverride) {
                paintOverride(painter, i, bgval);
            } else {
                painter.drawText(i.point, i.text);
            }
        }

        return;
    }

    // Doublesize text
    qreal x = i.rect.x();
    qreal y = i.rect.y();

    if (i.lineFlags & Tsq::DblTopLine) {
        painter.scale(2.0, 1.0);
        painter.setClipRect(i.rect);
        painter.translate(x, y);
        painter.scale(1.0, 2.0);
        painter.translate(-x, -y);
    }
    else if (i.lineFlags & Tsq::DblBottomLine) {
        painter.scale(2.0, 1.0);
        painter.setClipRect(i.rect);
        painter.translate(x, y - m_cellSize.height());
        painter.scale(1.0, 2.0);
        painter.translate(-x, -y);
    }
    else {
        painter.translate(0, y);
        painter.scale(2.0, 1.0);
        painter.translate(0, -y);
    }

    if (i.flags & Tsqt::ThumbFill)
        painter.fillRect(i.rect, QColor(bgval));

    if (!(i.flags & state.invisibleFlags)) {
        if (i.flags & Tsqt::PaintOverride) {
            paintOverride(painter, i, bgval);
        } else {
            painter.drawText(i.point, i.text);
        }
    }

    painter.setClipping(false);
    painter.resetTransform();
    painter.scale(state.scale, state.scale);
}

qreal
DisplayIterator::stringPixelWidth(const QString &text) const
{
    std::string str = text.toStdString();
    const char *cstr = str.c_str();
    Tsq::EmojiWalk ebf(m_term->unicoding(), str);
    Tsq::CellFlags flags;
    qreal width = 0;

    while (ebf.next(flags)) {
        if (flags) {
            width += 2 * m_cellSize.width();
        } else {
            auto tmp = QString::fromUtf8(cstr + ebf.start(), ebf.end() - ebf.start());
            width += m_metrics.width(tmp);
        }
    }

    return width;
}

column_t
DisplayIterator::stringCellWidth(const QString &text) const
{
    std::string str = text.toStdString();
    Tsq::CategoryWalk cbf(m_term->unicoding(), str);
    Tsq::CellFlags flags;
    column_t size, total = 0;

    while ((size = cbf.next(flags))) {
        total += size * (1 + !!(flags & Tsq::DblWidthChar));
    }

    return total;
}

qreal
DisplayIterator::decomposeStringPixels(DisplayCell &dc) const
{
    std::string str = dc.text.toStdString();
    for (char &c: str)
        if ((unsigned char)c < 0x20)
            c = ' ';

    const char *cstr = str.c_str();
    Tsq::EmojiWalk ebf(m_term->unicoding(), str);
    Tsq::CellFlags flags;

    qreal totalWidth = 0;

    while (ebf.next(flags)) {
        qreal curWidth;

        if (flags) {
            dc.text = QString::fromStdString(ebf.getEmojiName());
            curWidth = m_cellSize.width() * 2;
            dc.flags |= Tsq::EmojiChar;
        } else {
            dc.text = QString::fromUtf8(cstr + ebf.start(), ebf.end() - ebf.start());
            curWidth = m_metrics.width(dc.text);
        }

        dc.rect.setWidth(curWidth);
        m_displayCells.emplace_back(dc);

        dc.flags &= ~(Tsqt::CellFlags)Tsq::EmojiChar;
        dc.rect.translate(curWidth, 0);
        dc.point.rx() += curWidth;
        totalWidth += curWidth;
    }

    return totalWidth;
}

bool
DisplayIterator::decomposeStringCells(DisplayCell &dc, size_t endptr)
{
    std::string str = dc.substr.substr(0, endptr);
    const char *cstr = str.c_str();
    Tsq::DoubleWidthWalk dbf(m_term->unicoding(), str);
    Tsq::CellFlags flags;
    bool ulNeeded = false;
    column_t size;

    while ((size = dbf.next(flags))) {
        dc.flags |= flags;

        if (flags & Tsq::EmojiChar) {
            dc.text = QString::fromStdString(dbf.getEmojiName());
            ulNeeded = true;
        } else {
            dc.text = QString::fromUtf8(cstr + dbf.start(), dbf.end() - dbf.start());
        }
        if (flags & Tsq::DblWidthChar) {
            size = 2;
            ulNeeded = true;
        }

        qreal curWidth = size * m_cellSize.width();
        dc.rect.setWidth(curWidth);
        m_displayCells.emplace_back(dc);

        dc.flags &= ~(Tsqt::CellFlags)(Tsq::DblWidthChar|Tsq::EmojiChar);
        dc.rect.translate(curWidth, 0);
        dc.point.rx() += curWidth;
    }

    return ulNeeded;
}

unsigned
DisplayIterator::decomposeStringCells(DisplayCell &dc, unsigned maxCells, int *state)
{
    for (char &c: dc.substr)
        if ((unsigned char)c < 0x20)
            c = ' ';

    if (*state) {
        dc.rect.translate(-m_cellSize.width(), 0);
        dc.point.rx() -= m_cellSize.width();
    }

    std::string::const_iterator cur, next, end;
    unsigned usedCells = 0;
    next = dc.substr.begin(), end = dc.substr.end();
    auto *unicoding = m_term->unicoding();

    while (usedCells < maxCells && next != end) {
        cur = next;
        usedCells += unicoding->widthNext(next, end);
    }

    qreal ulsave = dc.point.x();

    if (decomposeStringCells(dc, next - dc.substr.begin()))
    {
        // Insert a separate cell of spaces for underlining
        DisplayCell ul(dc);
        ul.flags |= Tsqt::UnderlineDummy;
        ul.flags &= ~Tsqt::VisibleBg;
        ul.text = L(" ").repeated(usedCells);
        ul.rect.setX(ulsave);
        ul.rect.setWidth(usedCells * m_cellSize.width());
        ul.point.setX(ulsave);
        m_displayCells.emplace_back(std::move(ul));
    }

    size_t endptr;

    if (usedCells > maxCells) {
        endptr = cur - dc.substr.begin();
        *state = 1;
    } else {
        endptr = next - dc.substr.begin();
        *state = 0;
    }

    dc.substr = dc.substr.substr(endptr);
    return usedCells;
}

void
DisplayIterator::elideCellsLeft(qreal actualWidth, qreal targetWidth)
{
    long i;
    for (i = m_displayCells.size() - 1; i >= 0; --i)
    {
        DisplayCell &dc = m_displayCells[i];

        if (dc.rect.width() > targetWidth) {
            if (dc.flags & Tsq::EmojiChar) {
                dc.flags &= ~(Tsqt::CellFlags)Tsq::EmojiChar;
                dc.text = L(" \u2026");
                dc.rect.translate(targetWidth - actualWidth, 0);
                dc.point.rx() += targetWidth - actualWidth;
            } else {
                dc.text = m_metrics.elidedText(dc.text, Qt::ElideLeft, targetWidth);
                qreal newWidth = m_metrics.width(dc.text);
                qreal delta = targetWidth - actualWidth + dc.rect.width() - newWidth;
                dc.rect.translate(delta, 0);
                dc.point.rx() += delta;
                dc.rect.setWidth(newWidth);
            }
            break;
        }
        else {
            dc.rect.translate(targetWidth - actualWidth, 0);
            dc.point.rx() += targetWidth - actualWidth;
            targetWidth -= dc.rect.width();
            actualWidth -= dc.rect.width();
        }
    }

    if (i > 0)
        m_displayCells.erase(m_displayCells.begin(), m_displayCells.begin() + i);
}
