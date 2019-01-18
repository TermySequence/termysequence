// Copyright © 2018 TermySequence LLC
//
// SPDX-License-Identifier: GPL-2.0-only

#include "common.h"
#include "selection.h"
#include "buffers.h"
#include "term.h"
#include "scrollport.h"
#include "lib/grapheme.h"

#include <cctype>

static inline bool safeIsspace(codepoint_t codepoint)
{
    return codepoint < 256 && isspace(codepoint);
}

Selection::Selection(TermBuffers *buffers) :
    Region(Tsqt::RegionSelection, buffers, INVALID_REGION_ID),
    m_buffers(buffers)
{
}

bool
Selection::containsRel(size_t offset) const
{
    return contains(offset + m_buffers->origin());
}

bool
Selection::containsRel(size_t offset, column_t col) const
{
    return contains(offset + m_buffers->origin(), col);
}

bool
Selection::isAfter(size_t offset) const
{
    return (offset + m_buffers->origin()) <= endRow;
}

inline bool
Selection::checkResult()
{
    return startRow <= endRow && endRow < m_buffers->last() &&
        !RegionBase::isEmpty();
}

void
Selection::reportResult()
{
    if (checkResult()) {
        hasAnchor = true;
        clipboardSelect();
        m_buffers->activateSelection();
        emit activated();
    } else {
        hasAnchor = false;
        m_buffers->deactivateSelection();
        emit deactivated();
    }
}

inline void
Selection::reportModified()
{
    clipboardSelect();
    m_buffers->reportRegionChanged();
    emit modified();
}

void
Selection::selectBuffer()
{
    size_t n = m_buffers->size() - 1;

    anchorRow = startRow = m_buffers->origin();
    endRow = startRow + n;
    anchorCol = startCol = 0;
    endCol = m_buffers->safeRow(n).size;

    reportResult();
}

void
Selection::selectView(const TermScrollport *scrollport)
{
    size_t h = scrollport->height();
    size_t n = scrollport->offset() + h;

    anchorRow = startRow = m_buffers->origin() + scrollport->offset();
    endRow = startRow + h - 1;
    anchorCol = startCol = 0;

    if (n > m_buffers->size()) {
        endRow -= n - m_buffers->size();
        n = m_buffers->size();
    }

    endCol = m_buffers->safeRow(n - 1).size;

    reportResult();
}

void
Selection::selectRegion(const RegionBase *region)
{
    index_t n = m_buffers->last();

    anchorRow = startRow = region->startRow;
    endRow = region->endRow;

    if (startRow >= n)
        anchorRow = startRow = n - 1;
    if (endRow >= n)
        endRow = n - 1;

    anchorCol = startCol = region->startCol;
    n = m_buffers->safeRow(startRow - m_buffers->origin()).size;
    if (startCol > n)
        anchorCol = startCol = n;

    endCol = region->endCol;
    n = m_buffers->safeRow(endRow - m_buffers->origin()).size;
    if (endCol > n)
        endCol = n;

    reportResult();
}

void
Selection::selectWordAt(const QPoint &p)
{
    anchorRow = startRow = endRow = p.y() + m_buffers->origin();

    const auto &crow = m_buffers->safeRow(p.y());
    Tsq::GraphemeWalk tbf(m_buffers->term()->unicoding(), crow.str);

    for (startCol = endCol = 0; endCol < p.x() && tbf.next(); ++endCol)
        if (safeIsspace(tbf.codepoint()))
            startCol = endCol + 1;

    anchorCol = startCol;

    if (!tbf.finished())
        for (; tbf.next() && !safeIsspace(tbf.codepoint()); ++endCol);

    reportResult();
}

QVector<std::pair<column_t,column_t>>
Selection::getWords(index_t row) const
{
    QVector<std::pair<column_t,column_t>> words;
    const auto &crow = m_buffers->safeRow(row - m_buffers->origin());
    Tsq::GraphemeWalk tbf(m_buffers->term()->unicoding(), crow.str);
    column_t end, start;
    bool rc;

    // Skip leading whitespace
    for (end = 0; (rc = tbf.next()) && safeIsspace(tbf.codepoint()); ++end);

    while (rc) {
        start = end;
        for (; tbf.next() && !safeIsspace(tbf.codepoint()); ++end);
        words.append(std::make_pair(start, ++end));
        for (; (rc = tbf.next()) && safeIsspace(tbf.codepoint()); ++end);
        ++end;
    }

    return words;
}

void
Selection::selectWord(int index)
{
    if (!hasAnchor)
        return;

    auto words = getWords(anchorRow);
    column_t start, end;

    if (index < 0 && words.size() >= -index) {
        const auto &ref = words[words.size() + index];
        start = ref.first, end = ref.second;
    }
    else if (index < words.size()) {
        const auto &ref= words[index];
        start = ref.first, end = ref.second;
    }
    else {
        start = end = 0;
    }

    startRow = endRow = anchorRow;
    anchorCol = startCol = start;
    endCol = end;
    reportResult();
}

void
Selection::moveForwardWord()
{
    if (!hasAnchor)
        return;

    auto words = getWords(anchorRow);
    int i = 0;

    if (words.isEmpty()) {
        anchorCol = startCol = endCol = 0;
        goto out;
    }

    // If whole line is selected, select the first word
    if (startRow != endRow || startCol != 0 || endCol != words.back().second) {
        for (i = 0; i < words.size(); ++i)
            if (anchorCol < words[i].second)
                break;

        i = (i < words.size() - 1) ? i + 1 : 0;
    }
    anchorCol = startCol = words[i].first;
    endCol = words[i].second;
out:
    startRow = endRow = anchorRow;
    reportResult();
}

void
Selection::moveBackWord()
{
    if (!hasAnchor)
        return;

    auto words = getWords(anchorRow);
    int i;

    if (words.isEmpty()) {
        anchorCol = startCol = endCol = 0;
        goto out;
    }

    for (i = 0; i < words.size(); ++i)
        if (anchorCol < words[i].second)
            break;

    i = i > 0 ? i - 1 : words.size() - 1;
    anchorCol = startCol = words[i].first;
    endCol = words[i].second;
out:
    startRow = endRow = anchorRow;
    reportResult();
}

std::pair<index_t,bool>
Selection::forwardWord(bool upper)
{
    index_t *row = upper ? &startRow : &endRow;
    bool rc = false;
    auto words = getWords(*row);
    int i;

    if (words.isEmpty())
        goto out;

    if (upper) {
        for (i = 0; i < words.size(); ++i)
            if (startCol < words[i].first)
                break;

        if (i == words.size())
            goto out;
        if (i < words.size() - 1 && startCol == words[i].first)
            ++i;
        if (startRow == endRow && words[i].first >= endCol)
            goto out;

        anchorCol = startCol = words[i].first;
    }
    else {
        for (i = 0; i < words.size(); ++i)
            if (endCol < words[i].second)
                break;

        anchorCol = i < words.size() ? words[i].second :
            m_buffers->safeRow(endRow - m_buffers->origin()).size;
        if (endCol == anchorCol)
            goto out;
        endCol = anchorCol;
    }

    anchorRow = *row;
    rc = true;
    reportModified(); // Assumed active
out:
    return std::make_pair(*row, rc);
}

std::pair<index_t,bool>
Selection::backWord(bool upper)
{
    index_t *row = upper ? &startRow : &endRow;
    bool rc = false;
    auto words = getWords(*row);
    int i;

    if (words.isEmpty())
        goto out;

    if (upper) {
        for (i = 0; i < words.size(); ++i)
            if (startCol < words[i].second)
                break;

        if (i == words.size() || (i && startCol <= words[i].first))
            --i;

        anchorCol = words[i].first;
        anchorCol = (startCol > anchorCol) ? anchorCol : 0;
        if (startCol == anchorCol)
            goto out;
        startCol = anchorCol;
    }
    else {
        for (i = 0; i < words.size(); ++i)
            if (endCol <= words[i].second)
                break;

        if (i-- == 0)
            goto out;
        if (startRow == endRow && words[i].second <= startCol)
            goto out;

        anchorCol = endCol = words[i].second;
    }

    anchorRow = *row;
    rc = true;
    reportModified(); // Assumed active
out:
    return std::make_pair(*row, rc);
}

std::pair<index_t,bool>
Selection::forwardChar(bool upper)
{
    column_t *col;
    index_t *row;
    bool rc = false;

    if (upper) {
        col = &startCol;
        row = &startRow;
        if (startRow == endRow && startCol + 1 >= endCol)
            goto out;
    } else {
        col = &endCol;
        row = &endRow;
    }

    if (*col < m_buffers->safeRow(*row - m_buffers->origin()).size) {
        anchorRow = *row;
        anchorCol = ++*col;
        rc = true;
        reportModified(); // Assumed active
    }
out:
    return std::make_pair(*row, rc);
}

std::pair<index_t,bool>
Selection::backChar(bool upper)
{
    column_t *col;
    index_t *row;
    bool rc = false;

    if (upper) {
        col = &startCol;
        row = &startRow;
    } else {
        col = &endCol;
        row = &endRow;
        if (startRow == endRow && startCol + 1 >= endCol)
            goto out;
    }

    if (*col) {
        anchorRow = *row;
        anchorCol = --*col;
        rc = true;
        reportModified(); // Assumed active
    }
out:
    return std::make_pair(*row, rc);
}

std::pair<index_t,bool>
Selection::forwardLine(bool upper)
{
    column_t *col = upper ? &startCol : &endCol, next;
    index_t *row = upper ? &startRow : &endRow;
    bool rc = false;

    if (*row == m_buffers->last() - 1)
        goto out;

    next = m_buffers->xByPos(*row, *col);
    next = m_buffers->posByX(*row + 1 - m_buffers->origin(), next);

    if (upper) {
        if (startRow == endRow)
            goto out;
        if (startRow + 1 == endRow && next >= endCol)
            goto out;
    }

    anchorRow = ++*row;
    anchorCol = *col = next;
    rc = true;
    reportModified(); // Assumed active
out:
    return std::make_pair(*row, rc);
}

std::pair<index_t,bool>
Selection::backLine(bool upper)
{
    column_t *col = upper ? &startCol : &endCol, next;
    index_t *row = upper ? &startRow : &endRow;
    bool rc = false;

    if (*row == m_buffers->origin())
        goto out;

    next = m_buffers->xByPos(*row, *col);
    next = m_buffers->posByX(*row - 1 - m_buffers->origin(), next);

    if (!upper) {
        if (startRow == endRow)
            goto out;
        if (startRow + 1 == endRow && next <= startCol)
            goto out;
    }

    anchorRow = --*row;
    anchorCol = *col = next;
    rc = true;
    reportModified(); // Assumed active
out:
    return std::make_pair(*row, rc);
}

void
Selection::selectLineAt(const QPoint &p)
{
    anchorRow = startRow = endRow = p.y() + m_buffers->origin();
    anchorCol = startCol = 0;
    endCol = m_buffers->safeRow(p.y()).size;
    reportResult();
}

void
Selection::selectLine(int arg)
{
    if (!hasAnchor)
        return;

    column_t size = m_buffers->safeRow(endRow - m_buffers->origin()).size;

    switch (arg) {
    case 0:
        anchorCol = startCol = 0;
        endCol = size;
        break;
    case 1:
        anchorCol = startCol = 0;
        break;
    case 2:
        endCol = size;
        break;
    }

    reportResult();
}

void
Selection::setAnchor(const QPoint &p)
{
    anchorRow = p.y() + m_buffers->origin();
    anchorCol = p.x();

    startRow = endRow = anchorRow;
    startCol = endCol = anchorCol;

    hasAnchor = true;

    m_buffers->deactivateSelection();
    emit restarted();
}

void
Selection::moveAnchor(bool start)
{
    anchorRow = start ? startRow : endRow;
    anchorCol = start ? startCol : endCol;
}

bool
Selection::setFloat(const QPoint &p)
{
    if (!hasAnchor)
        return false;

    index_t row = p.y() + m_buffers->origin();
    column_t col = p.x();
    index_t nextStartRow, nextEndRow;
    column_t nextStartCol, nextEndCol;

    if (row == anchorRow) {
        nextStartRow = nextEndRow = row;
        if (col < anchorCol) {
            nextStartCol = col;
            nextEndCol = anchorCol;
        } else {
            nextStartCol = anchorCol;
            nextEndCol = col;
        }
    }
    else if (row < anchorRow) {
        nextStartRow = row;
        nextStartCol = col;
        nextEndRow = anchorRow;
        nextEndCol = anchorCol;
    }
    else {
        nextStartRow = anchorRow;
        nextStartCol = anchorCol;
        nextEndRow = row;
        nextEndCol = col;
    }

    size_t n = nextEndRow + 1 - m_buffers->origin();
    if (n > m_buffers->size())
        nextEndRow -= (n - m_buffers->size());

    n = m_buffers->safeRow(nextStartRow - m_buffers->origin()).size;
    if (nextStartCol > n)
        nextStartCol = n;
    n = m_buffers->safeRow(nextEndRow - m_buffers->origin()).size;
    if (nextEndCol > n)
        nextEndCol = n;

    bool changed = nextStartRow != startRow || nextEndRow != endRow ||
        nextStartCol != startCol || nextEndCol != endCol;

    if (changed) {
        startRow = nextStartRow;
        endRow = nextEndRow;
        startCol = nextStartCol;
        endCol = nextEndCol;
    }

    bool rc = checkResult();
    if (!rc) {
        m_buffers->deactivateSelection();
        emit modified();
    } else if (changed) {
        m_buffers->activateSelection();
        emit modified();
    }
    return rc;
}

void
Selection::finish(const QPoint &p)
{
    if (setFloat(p)) {
        clipboardSelect();
        emit activated();
    } else {
        emit deactivated();
    }
}

void
Selection::clear()
{
    hasAnchor = false;
    startRow = endRow = INVALID_INDEX;
    startCol = endCol = INVALID_COLUMN;

    m_buffers->deactivateSelection();
    emit deactivated();
}

QString
Selection::getLine() const
{
    return hasAnchor ? Region::getLine() : QString();
}
