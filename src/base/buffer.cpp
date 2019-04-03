// Copyright © 2018 TermySequence LLC
//
// SPDX-License-Identifier: GPL-2.0-only

#include "common.h"
#include "app/attr.h"
#include "app/config.h"
#include "app/exception.h"
#include "app/logging.h"
#include "buffer.h"
#include "buffers.h"
#include "term.h"
#include "selection.h"
#include "builder.h"
#include "listener.h"
#include "sembase.h"
#include "lib/grapheme.h"
#include "lib/utf8.h"

#include <QSet>
#include <regex>
#include <cassert>

static const std::regex s_urlre(
    "\\b(?:(?:https?|ftp|file)://|info:|man:)"
    "(?:\\([-_[:alnum:]+&@#/%=~|$?!:,.\\\\]*\\)|[-_[:alnum:]+&@#/%=~|$?!:,.\\\\])*"
    "(?:\\([-_[:alnum:]+&@#/%=~|$?!:,.\\\\]*\\)|[_[:alnum:]+&@#/%=~|$\\\\])",
    std::regex::nosubs|std::regex::optimize|std::regex::icase
    );

TermBuffer::TermBuffer(TermBuffers *parent, uint8_t bufid):
    BufferBase(parent->term()),
    m_parent(parent),
    m_bufid(bufid)
{
}

TermBuffer::TermBuffer():
    BufferBase(nullptr)
{
    // TODO Remove this once clang fixes its array initialization bug
    assert(0);
}

TermBuffer::~TermBuffer()
{
    for (auto &&i: qAsConst(m_regions))
        delete i.second;
    for (auto &&i: qAsConst(m_semantics))
        delete i.second;
}

inline CellRow &
TermBuffer::row(size_t i)
{
    return m_rows[(m_origin + i) & m_capmask];
}

inline Region *
TermBuffer::safeRegion_(regionid_t id) const
{
    auto i = m_regions.find(id);
    return i != m_regions.end() ? i->second : nullptr;
}

inline Region *
TermBuffer::safeSemantic_(regionid_t id) const
{
    auto i = m_semantics.find(id);
    return i != m_semantics.end() ? i->second : nullptr;
}

inline Tsqt::CellFlags
TermBuffer::regionFlags(index_t index, column_t pos) const
{
    Tsqt::CellFlags result = 0;

    for (auto r: qAsConst(m_activeRegions))
    {
        if (r->contains(index, pos)) {
            switch (r->type()) {
            case Tsqt::RegionUser:
                result |= Tsqt::Annotation;
                break;
            case Tsqt::RegionPrompt:
                result |= Tsqt::ActivePrompt;
                break;
            case Tsqt::RegionSearch:
                result |= Tsq::Bold;
                break;
            default:
                break;
            }
        }
    }

    return result;
}

Cell
TermBuffer::cellByPos(size_t idx, column_t pos) const
{
    // Note: This method intended for finding cursor cell only
    // Find the attributes and horizontal position
    const CellRow &crow = row(idx);
    const CellRange *rcur = crow.ranges.data();
    const CellRange *rend = rcur + crow.ranges.size();

    Cell cell;
    cell.cellx = pos;
    cell.cellwidth = 1;

    while (rcur < rend) {
        if (rcur->start > pos) {
            break;
        }
        if (rcur->end >= pos) {
            if (rcur->flags & Tsq::DblWidthChar) {
                cell.cellx += pos - rcur->start;
                cell.cellwidth = 2;
            }
            cell.CellAttributes::operator=(*rcur);
            break;
        }
        if (rcur->flags & Tsq::DblWidthChar)
            cell.cellx += rcur->end - rcur->start + 1;
        ++rcur;
    }

    // Find the substring
    Tsq::GraphemeWalk tbf(m_term->unicoding(), crow.str);
    codepoint_t val = 0;

    for (column_t i = 0; i < pos && tbf.next(); ++i);

    if (!tbf.finished()) {
        tbf.next();
        cell.startptr = tbf.start();
        cell.endptr = tbf.end();
        val = tbf.codepoint();
    }

    // Extra flags...
    // Special unicode ranges
    if (val >= 0x2500 && val <= 0x259f)
        cell.flags |= Tsqt::PaintU2500;
    // Within selection
    if (m_selection && m_selection->containsRel(m_lower + idx, pos) && pos < crow.size)
        cell.flags |= (crow.flags & Tsqt::NoSelect) ? 0 : Tsqt::Selected;
    // Other regions, annotations
    if (!m_activeRegions.empty())
        cell.flags |= regionFlags(m_origin + idx, pos);

    return cell;
}

column_t
TermBuffer::posByX(size_t idx, int x) const
{
    const CellRow &crow = row(idx);
    const CellRange *rcur = crow.ranges.data();
    const CellRange *rend = rcur + crow.ranges.size();

    if (crow.flags & Tsq::DblWidthLine)
        x /= 2;

    column_t pos = 0;
    int posx = 0;

    while (rcur < rend) {
        int startx = posx + (rcur->start - pos);

        if (x < startx) {
            pos += x - posx;
            posx += x - posx;
            break;
        }

        pos = rcur->start;
        posx = startx;

        int w = 1 + !!(rcur->flags & Tsq::DblWidthChar);
        int endx = startx + (rcur->end - rcur->start + 1) * w;

        if (x >= startx && x < endx) {
            pos += (x - startx) / w;
            posx += x - startx;
            break;
        }

        pos = rcur->end + 1;
        posx = endx;
        ++rcur;
    }

    pos += x - posx;
    return (pos <= crow.size) ? pos : crow.size;
}

Cell
TermBuffer::cellByX(size_t idx, int x) const
{
    // Note: This method intended for finding mouse cell only
    // Find the attributes and horizontal position
    const CellRow &crow = row(idx);
    const CellRange *rcur = crow.ranges.data();
    const CellRange *rend = rcur + crow.ranges.size();

    Cell cell;
    column_t pos = 0;
    int posx = 0;

    while (rcur < rend) {
        int startx = posx + (rcur->start - pos);

        if (x < startx) {
            pos += x - posx;
            posx += x - posx;
            break;
        }

        pos = rcur->start;
        posx = startx;

        int w = 1 + !!(rcur->flags & Tsq::DblWidthChar);
        int endx = startx + (rcur->end - rcur->start + 1) * w;

        if (x >= startx && x < endx) {
            pos += (x - startx) / w;
            posx += x - startx;
            cell.CellAttributes::operator=(*rcur);
            break;
        }

        pos = rcur->end + 1;
        posx = endx;
        ++rcur;
    }

    pos += x - posx;

    // Extra flags...
    // Within selection
    if (m_selection && m_selection->containsRel(m_lower + idx, pos) && pos < crow.size)
        cell.flags |= Tsqt::Selected;
    // Search match
    if ((crow.flags & Tsqt::SearchHit) && m_term->searching())
        cell.flags |= (pos >= crow.matchStart && pos < crow.matchEnd) ?
            Tsqt::SearchText : Tsqt::SearchLine;
    // Other regions, annotations
    if (!m_activeRegions.empty())
        cell.flags |= regionFlags(m_origin + idx, pos);

    return cell;
}

/*
  TODO: deal with the proliferation of position converter functions
  Buffer:
    x<-pos (row, pos) (private)
    x<-ptr (index, ptr) (private)
    cell<-pos (offset, pos)
    cell<-x   (offset, x)
    pos<-x    (offset, pos)
    x<-pos    (offset, x)
    xsize     (offset)
    x<-js     (offset, js)
  Buffers:
    cell<-pos (offset, pos)
    cell<-x   (offset, x)
    pos<-x    (offset, x)
    x<-pos    (index, pos)
    xsize     (index)
  Sembase:
    jssize    (str)
  FontBase:
    xsize     (qstr)
    pixelsize (qstr, metrics)
  JsApi:
    pos<-js   (offset, js)
    js<-pos   (offset, pos)
    ptr<-pos  (str, pos)
    xsize     (str)
    possize   (str)
*/
column_t
TermBuffer::xByPos(const CellRow &crow, column_t pos) const
{
    const CellRange *rcur = crow.ranges.data();
    const CellRange *rend = rcur + crow.ranges.size();

    column_t x = pos;

    while (rcur < rend) {
        if (rcur->start > pos) {
            break;
        }
        if (rcur->end >= pos) {
            if (rcur->flags & Tsq::DblWidthChar) {
                x += pos - rcur->start;
            }
            break;
        }
        if (rcur->flags & Tsq::DblWidthChar)
            x += rcur->end - rcur->start + 1;
        ++rcur;
    }

    if (crow.flags & Tsq::DblWidthLine)
        x *= 2;

    return x;
}

column_t
TermBuffer::xByPos(size_t idx, column_t pos) const
{
    return xByPos(row(idx), pos);
}

column_t
TermBuffer::xSize(size_t idx) const
{
    const CellRow &crow = row(idx);
    return xByPos(crow, crow.size);
}

column_t
TermBuffer::xByPtr(index_t idx, size_t ptr) const
{
    const CellRow &crow = m_rows[idx & m_capmask];
    column_t x = 0;
    const char *start, *next, *end;
    next = start = crow.str.data(), end = start + crow.str.size();
    auto *unicoding = m_term->unicoding();

    while (next - start < ptr && next != end) {
        x += unicoding->widthNext(next, end);
    }

    return x;
}

column_t
TermBuffer::xByJavascript(index_t idx, size_t jspos) const
{
    const CellRow &crow = m_rows[idx & m_capmask];
    size_t curpos = 0;
    const char *start, *next, *end;
    next = start = crow.str.data(), end = start + crow.str.size();

    while (curpos < jspos && next != end) {
        codepoint_t val = utf8::unchecked::next(next);
        curpos += 1 + (val > 0xffff);
    }

    auto *unicoding = m_term->unicoding();
    column_t x = 0;

    while (start != next) {
        x += unicoding->widthNext(start, next);
    }

    return x;
}

void
TermBuffer::deleteSemanticRegions(Region *r)
{
    RegionBase lower(Tsqt::RegionLink);
    lower.startRow = r->startRow;

    auto i = m_regionsByStart.lower_bound(&lower);

    while (i != m_regionsByStart.end() && i->ptr->startRow <= r->endRow) {
        Region *region = i->ref();
        if (region->type() == Tsqt::RegionSemantic && region->parent == r->id()) {
            m_semantics.erase(region->id());
            i = m_regionsByStart.erase(i);
            // SemanticRegion not stored in m_regionsByEnd
            delete region;
        } else {
            ++i;
        }
    }
}

inline void
TermBuffer::deleteRegion(Region *region, bool update)
{
    bool issem = false;

    // Special handling for certain region types
    switch (region->type()) {
    case Tsqt::RegionPrompt:
        m_parent->reportPromptDeleted(region->id());
        break;
    case Tsqt::RegionOutput:
        if (region->parser) {
            region->parser->destroy();
            if (update)
                deleteSemanticRegions(region);
        }
        break;
    case Tsqt::RegionUser:
        m_parent->reportNoteDeleted(region);
        break;
    case Tsqt::RegionImage:
        m_term->putImage(region);
        break;
    case Tsqt::RegionLink:
    case Tsqt::RegionSemantic:
        issem = true;
        break;
    default:
        break;
    }

    if (update) {
        (issem ? m_semantics : m_regions).erase(region->id());
        m_activeRegions.erase(region);
        m_regionsByStart.erase(region);
        m_regionsByEnd.erase(region);
    }

    delete region;
}

void
TermBuffer::deleteSemantic(regionid_t id)
{
    Region *r = safeSemantic_(id);
    if (r) {
        deleteRegion(r, true);
        m_parent->reportRegionChanged();
    }
}

inline void
TermBuffer::handleJobRegion(Region *r)
{
    if ((r->flags & (Tsq::HasCommand|Tsq::EmptyCommand)) == Tsq::HasCommand &&
        r->attributes.contains(g_attr_REGION_STARTED))
        m_parent->reportJobChanged(r);
}

inline void
TermBuffer::handleOutputRegion(Region *r, bool isNew)
{
    if (isNew) {
        const Region *job = safeRegion(r->parent);
        if (job)
            r->parser = SemanticFeature::createParser(this, job, r);
    }
    if (r->parser && !r->parser->setRegion()) {
        // Remove all semantic regions
        deleteSemanticRegions(r);
    }
}

void
TermBuffer::handleUserRegion(Region *r)
{
    // Find existing overlapping regions
    RegionBase lower(Tsqt::RegionLowerBound);
    lower.startRow = lower.endRow = r->startRow;
    RegionBase upper(Tsqt::RegionUpperBound);
    upper.startRow = upper.endRow = r->endRow;

    auto i = m_regionsByStart.lower_bound(&lower);
    auto j = m_regionsByStart.upper_bound(&upper);
    auto k = m_regionsByEnd.lower_bound(&lower);
    auto l = m_regionsByEnd.upper_bound(&upper);

    QSet<Region*> deleteRegions;

    while (i != j) {
        if (i->ptr->type() == Tsqt::RegionUser && i->cref() != r && r->overlaps(i->ptr)) {
            deleteRegions.insert(i->ref());
        }
        ++i;
    }
    while (k != l) {
        if (k->ptr->type() == Tsqt::RegionUser && k->cref() != r && r->overlaps(k->ptr)) {
            deleteRegions.insert(k->ref());
        }
        ++k;
    }

    for (auto i: qAsConst(deleteRegions))
        deleteRegion(i, true);

    activateRegion(r);

    if (r->attributes.contains(g_attr_REGION_STARTED))
        m_parent->reportNoteChanged(r);
}

inline void
TermBuffer::handleImageRegion(Region *r, bool isNew)
{
    if (isNew)
        m_term->registerImage(r);
}

static inline void
decodeAttribute(Region *r, const QString &fromKey, const QString &toKey)
{
    QString val;

    if (!(val = r->attributes.value(fromKey)).isEmpty()) {
        QByteArray buf = QByteArray::fromBase64(val.toLatin1());
        r->attributes[toKey] = QString::fromUtf8(buf.data(), buf.size());
    }
}

void
TermBuffer::handleContentRegion(Region *r)
{
    if (r->attributes.value(g_attr_CONTENT_TYPE) == A("515")) {
        decodeAttribute(r, g_attr_CONTENT_ICON, g_attr_SEM_ICON);
        decodeAttribute(r, g_attr_CONTENT_MENU, g_attr_SEM_MENU);
        decodeAttribute(r, g_attr_CONTENT_DRAG, g_attr_SEM_DRAG);
        decodeAttribute(r, g_attr_CONTENT_ACTION1, g_attr_SEM_ACTION1);
        decodeAttribute(r, g_attr_CONTENT_TOOLTIP, g_attr_SEM_TOOLTIP);
    }
    else {
        QString uri = r->attributes.value(g_attr_CONTENT_URI);
        r->attributes[g_attr_SEM_ICON] = uri.startsWith("file:") ? A("file") : A("link");
    }
}

void
TermBuffer::updateRow(CellRow &row, index_t index)
{
    CellBuilder builder(row);
    CellAttributes attr;

    row.cells.clear();
    row.regionState = m_parent->regionState();

    const CellRange *rcur = row.ranges.data();
    const CellRange *rend = rcur + row.ranges.size();

    Tsq::GraphemeWalk tbf(m_term->unicoding(), row.str);
    column_t pos = 0;

    while (tbf.next()) {
        // Get cell attribute for current character
        if (rcur == rend || rcur->start > pos) {
            // Current character is not in any range
            attr = CellAttributes();
        }
        else {
            // Use rcur->attr
            attr = *rcur;

            if (rcur->end == pos) {
                ++rcur;
            }
        }

        // Extra flags...
        codepoint_t val = tbf.codepoint();
        // Special unicode ranges
        if (val >= 0x2500 && val <= 0x259f)
            attr.flags |= Tsqt::PaintU2500;
        // Within selection
        if (m_selection && m_selection->containsRel(m_lower + index - m_origin, pos) &&
            (row.flags & Tsqt::NoSelect) == 0)
            attr.flags |= Tsqt::Selected;
        // Search match
        if ((row.flags & Tsqt::SearchHit) && m_term->searching())
            attr.flags |= (pos >= row.matchStart && pos < row.matchEnd) ?
                Tsqt::SearchText : Tsqt::SearchLine;
        // Other regions, annotations
        if (!m_activeRegions.empty())
            attr.flags |= regionFlags(index, pos);

        // Push
        builder.visit(attr, attr.isVisible(val), pos, tbf.start(), tbf.end());
        ++pos;
    }

    builder.finish();
    row.size = pos;
}

bool
TermBuffer::searchRow(CellRow &row)
{
    bool changed = false;
    bool hit = false;
    std::smatch match;

    if (m_term->searching()) {
        hit = std::regex_search(row.str, match, m_term->search().regex);
    }

    if (hit) {
        size_t startptr = match.position();
        size_t endptr = startptr + match.length();
        uint16_t pos = 0, matchStart = 0, matchEnd = row.size;

        Tsq::GraphemeWalk tbf(m_term->unicoding(), row.str);

        while (tbf.next()) {
            if (tbf.start() >= startptr) {
                matchStart = pos;
                break;
            }
            ++pos;
        }
        while (tbf.next()) {
            ++pos;
            if (tbf.start() >= endptr) {
                matchEnd = pos;
                break;
            }
        }

        changed = (row.flags & Tsqt::SearchHit) == 0 ||
            row.matchStart != matchStart ||
            row.matchEnd != matchEnd;

        if (changed) {
            row.flags |= Tsqt::SearchHit;
            row.matchStart = matchStart;
            row.matchEnd = matchEnd;
        }
    }
    else if ((changed = (row.flags & Tsqt::SearchHit))) {
        row.flags &= ~Tsqt::SearchHit;
    }

    return changed;
}

void
TermBuffer::updateRows(size_t start, size_t end, RegionList *regionret)
{
    index_t rownum = m_origin + start;

    if (regionret) {
        RegionBase lower(Tsqt::RegionLowerBound);
        lower.startRow = lower.endRow = rownum;
        RegionBase upper(Tsqt::RegionUpperBound);
        upper.startRow = upper.endRow = m_origin + end - 1;

        auto i = m_regionsByEnd.lower_bound(&lower);
        auto j = m_regionsByEnd.upper_bound(&upper);

        for (; i != j; ++i)
            if (i->cref()->type() == Tsqt::RegionImage && i->cref()->startRow < rownum)
                regionret->list.push_back(*i);

        i = m_regionsByStart.lower_bound(&lower);
        j = m_regionsByStart.upper_bound(&upper);

        for (; i != j; ++i)
            if (i->cref()->flags & Tsqt::Updating)
                regionret->list.push_back(*i);
    }

    for (; start < end; ++start, ++rownum)
    {
        CellRow &r = row(start);
        if (r.regionState != m_parent->regionState())
            updateRow(r, rownum);
    }
}

index_t
TermBuffer::searchRows(size_t start, size_t end)
{
    index_t rownum = m_origin + start;
    index_t result = INVALID_INDEX;

    for (; start < end; ++start, ++rownum)
    {
        CellRow &r = row(start);
        if (searchRow(r))
            updateRow(r, rownum);
        if (r.flags & Tsqt::SearchHit)
            result = rownum;
    }

    return result;
}

void
TermBuffer::fetchRows(size_t start, size_t end)
{
    index_t startnum = m_origin + start;
    index_t endnum = m_origin + end - 1;
    size_t i, j;

    for (i = start; i < end; ++i, ++startnum)
    {
        CellRow &r = row(i);
        if ((r.flags & Tsqt::Downloaded) == 0)
            break;
    }
    for (j = end - 1; j >= i; --j, --endnum)
    {
        CellRow &r = row(j);
        if ((r.flags & Tsqt::Downloaded) == 0)
        {
            g_listener->pushTermFetch(m_term, startnum, endnum, m_bufid);
            break;
        }
    }
}

void
TermBuffer::setRow(index_t index, const uint32_t *data, const char *str, unsigned len, bool pushed)
{
    if (index >= m_size)
        return;
    if (index < m_updatelo)
        m_updatelo = index;
    if (index > m_updatehi)
        m_updatehi = index;

    CellRow &row = m_rows[index & m_capmask];
    const uint32_t *ptr;
    unsigned i, prevend;

    if (m_selection && pushed && m_selection->containsRel(m_lower + index - m_origin))
        m_selection->clear();

    if (!utf8::is_valid(str, str + len))
        throw TsqException("Unmarshal failed: invalid row UTF-8");

    row.str.assign(str, len);
    row.ranges.clear();
    row.flags = (data[0] & Tsq::ServerLineMask) | (pushed ? 0 : Tsqt::Downloaded);
    row.modtime = data[1];

    for (i = 0, ptr = data + 3; i < data[2]; ++i) {
        if (ptr[0] > ptr[1] || (i && ptr[0] <= prevend))
            throw TsqException("Unmarshal failed: invalid row range");

        prevend = ptr[1];
        row.ranges.emplace_back(ptr);
        ptr += 6;
    }

    if (m_term->searching())
        searchRow(row);

    updateRow(row, index);
}

void
TermBuffer::setRegion(const uint32_t *data, AttributeMap &attributes)
{
    auto id = data[0];
    Tsqt::RegionType type = (Tsqt::RegionType)(data[1] >> 8 & 0xff);
    auto flags = data[2] & ~Tsq::LocalRegionMask;
    auto parent = data[3];
    index_t startRow = data[4] | (index_t)data[5] << 32;
    index_t endRow = data[6] | (index_t)data[7] << 32;
    auto startCol = data[8];
    auto endCol = data[9];

    if (id == INVALID_REGION_ID)
        return;

    Region *r = safeRegion_(id);
    bool haveRegion = !!r;
    bool insertStart, insertEnd;

    if (flags & Tsq::Deleted) {
        if (haveRegion) {
            deleteRegion(r, true);
        }
        return;
    } else if (haveRegion) {
        r->flags = flags | (r->flags & Tsq::LocalRegionMask);

        if ((insertStart = startRow != r->startRow || startCol != r->startCol)) {
            m_regionsByStart.erase(r);
            r->startRow = startRow;
            r->startCol = startCol;
        }
        if ((insertEnd = endRow != r->endRow || endCol != r->endCol)) {
            m_regionsByEnd.erase(r);
            r->endRow = endRow;
            r->endCol = endCol;
        }
    } else {
        r = new Region(type, this, id);
        m_regions[id] = r;

        r->parent = parent;
        r->flags = flags;
        r->startRow = startRow;
        r->endRow = endRow;
        r->startCol = startCol;
        r->endCol = endCol;

        insertStart = insertEnd = true;
    }

    r->attributes.swap(attributes);

    if (insertStart)
        m_regionsByStart.insert(r);
    if (insertEnd)
        m_regionsByEnd.insert(r);

    // Special handling for certain region types
    switch (type) {
    case Tsqt::RegionJob:
        handleJobRegion(r);
        r->flags |= Tsqt::Updating;
        break;
    case Tsqt::RegionOutput:
        handleOutputRegion(r, !haveRegion);
        r->flags |= Tsqt::Updating;
        break;
    case Tsqt::RegionUser:
        handleUserRegion(r);
        r->flags |= Tsqt::Updating|Tsqt::Inline;
        break;
    case Tsqt::RegionImage:
        handleImageRegion(r, !haveRegion);
        r->flags |= Tsqt::Updating|Tsqt::Inline;
        break;
    case Tsqt::RegionContent:
        if (!haveRegion)
            handleContentRegion(r);
        break;
    default:
        break;
    }
}

void
TermBuffer::changeLength(index_t size)
{
    if (m_size < size) {
        // check selection
        if (m_selection && m_selection->containsRel(m_lower + m_size - m_origin)) {
            m_selection->clear();
        }
        index_t startpos = m_size & m_capmask;
        do {
            if (m_size < m_capacity) {
                m_rows.emplace_back();
            } else {
                m_rows[m_size & m_capmask].clear();
                ++m_origin;
            }
            if ((++m_size & m_capmask) == startpos) {
                m_origin += size - m_size;
                m_size = size;
                break;
            }
        }
        while (m_size < size);

        if (m_origin) {
            while (!m_regionsByStart.empty() &&
                   m_regionsByStart.begin()->ptr->startRow < m_origin)
            {
                deleteRegion(m_regionsByStart.begin()->ref(), true);
            }
        }
    }
    else {
        // check selection
        if (m_selection && m_selection->isAfter(m_lower + size - m_origin)) {
            m_selection->clear();
        }
        if (m_size - size >= m_capacity) {
            if (size < m_capacity) {
                m_rows.resize(size);
                m_origin = 0;
            } else {
                m_origin -= m_size - size;
            }
            for (auto &row: m_rows)
                row.clear();
            m_size = size;
            return;
        }
        while (m_size > size)
        {
            if (m_size > m_capacity) {
                m_rows[--m_origin & m_capmask].clear();
                --m_size;
            } else {
                m_rows.resize(size);
                m_size = size;
                m_origin = 0;
                break;
            }
        }
    }
}

bool
TermBuffer::changeCapacity(index_t size, uint8_t capspec)
{
    uint8_t caporder = capspec & 0x7f;
    m_noScrollback = !!(capspec & 0x80);

    if (m_selection)
        m_selection->clear();

    // Delete all regions
    for (auto &&i: qAsConst(m_regions))
        deleteRegion(i.second, false);
    for (auto &&i: qAsConst(m_semantics))
        deleteRegion(i.second, false);

    m_regions.clear();
    m_semantics.clear();
    m_activeRegions.clear();
    m_regionsByStart.clear();
    m_regionsByEnd.clear();

    if (m_caporder < caporder) {
        // Increase size
        index_t pos = m_size & m_capmask;

        if (pos < m_rows.size())
        {
            if (pos < m_capacity / 2) {
                // Move elements to back
                for (index_t i = 0; i < pos; ++i) {
                    CellRow save = std::move(m_rows.front());
                    m_rows.pop_front();
                    m_rows.emplace_back(std::move(save));
                }
            } else {
                // Move elements to front
                for (index_t i = pos; i < m_capacity; ++i) {
                    CellRow save = std::move(m_rows.back());
                    m_rows.pop_back();
                    m_rows.emplace_front(std::move(save));
                }
            }
        }
    } else {
        // Decrease size
        // Find the two ends
        index_t capacity = 1 << caporder;
        index_t end = m_size & m_capmask;
        index_t start = (m_size > capacity) ? (end - capacity) & m_capmask : 0;

        if (end >= start) {
            // Remove elements from back
            m_rows.erase(m_rows.begin() + end, m_rows.end());
            // Remove elements from front
            m_rows.erase(m_rows.begin(), m_rows.begin() + start);
        } else {
            // Remove elements from middle
            m_rows.erase(m_rows.begin() + end, m_rows.begin() + start);
            // Move elements to back
            for (index_t i = 0; i < end; ++i) {
                CellRow save = std::move(m_rows.front());
                m_rows.pop_front();
                m_rows.emplace_back(std::move(save));
            }
        }
    }

    // Force refetching of all scrollback contents
    for (auto &row: m_rows) {
        row.flags &= ~Tsqt::Downloaded;
    }

    // Set new size and capacity
    bool rc = (m_caporder != 0);
    m_size = m_rows.size();
    m_origin = 0;
    m_capacity = 1 << caporder;
    m_capmask = m_capacity - 1;
    m_caporder = caporder;

    changeLength(size);
    return rc;
}

const Region *
TermBuffer::firstRegion(Tsqt::RegionType type) const
{
    for (auto i: qAsConst(m_regionsByStart))
        if (i.ptr->type() == type)
            return i.cref();

    return nullptr;
}

const Region *
TermBuffer::lastRegion(Tsqt::RegionType type) const
{
    for (auto i = m_regionsByStart.rbegin(), j = m_regionsByStart.rend(); i != j; ++i)
        if (i->ptr->type() == type)
            return i->cref();

    return nullptr;
}

const Region *
TermBuffer::nextRegion(Tsqt::RegionType type, regionid_t id) const
{
    Region *cur = safeRegion_(id);

    auto i = cur ? m_regionsByStart.find(cur) : m_regionsByStart.begin();
    auto j = m_regionsByStart.end();

    if (i != j)
        while (++i != j)
            if (i->ptr->type() == type)
                return i->cref();

    return nullptr;
}

const Region*
TermBuffer::prevRegion(Tsqt::RegionType type, regionid_t id) const
{
    Region *cur = safeRegion_(id);

    auto i = m_regionsByStart.begin();
    auto j = cur ? m_regionsByStart.find(cur) : m_regionsByStart.end();

    if (i != j) {
        do {
            if ((--j)->ptr->type() == type)
                return j->cref();
        } while (i != j);
    }

    return nullptr;
}

const Region *
TermBuffer::downRegion(Tsqt::RegionType type, index_t row) const
{
    RegionBase lower(Tsqt::RegionLowerBound);
    lower.startRow = row;

    auto i = m_regionsByStart.lower_bound(&lower);
    auto j = m_regionsByStart.end();

    if (i != j) {
        do {
            if (i->ptr->type() == type)
                return i->cref();
        } while (++i != j);
    }

    return nullptr;
}

const Region*
TermBuffer::upRegion(Tsqt::RegionType type, index_t row) const
{
    RegionBase lower(Tsqt::RegionLowerBound);
    lower.startRow = row;

    auto i = m_regionsByStart.begin();
    auto j = m_regionsByStart.lower_bound(&lower);

    if (i != j) {
        do {
            if ((--j)->ptr->type() == type)
                return j->cref();
        } while (i != j);
    }

    return nullptr;
}

const Region *
TermBuffer::modtimeRegion() const
{
    const Region *g = nullptr;
    index_t max = origin() + size();

    for (auto i = m_regionsByStart.crbegin(), j = m_regionsByStart.crend(); i != j; ++i)
    {
        const Region *r = i->cref();

        if (r->startRow >= max) {
            // qCWarning(lcCommand, "Warning: region starts (%llu) past end (%llu)", r->startRow, max);
            continue;
        }

        if (r->type() == Tsqt::RegionCommand &&
            (r->flags & (Tsq::HasEnd|Tsq::EmptyCommand)) == Tsq::HasEnd)
            return r;

        if (!g && r->type() == Tsqt::RegionJob)
            g = r;
    }

    return g;
}

const Region *
TermBuffer::findRegionByRow(Tsqt::RegionType type, index_t row) const
{
    // Find the job region
    RegionBase lower(Tsqt::RegionLowerBound);
    lower.endRow = row;

    auto i = m_regionsByEnd.lower_bound(&lower);
    auto j = m_regionsByEnd.end();

    while (1) {
        if (i == j)
            return nullptr;
        else if (i->ptr->type() != Tsqt::RegionJob)
            ; // do nothing
        else if (i->ptr->startRow > row)
            return nullptr;
        else if (i->ptr->contains(row, 0))
            break;

        ++i;
    }

    const Region *job = i->cref();

    if (type == Tsqt::RegionJob)
        return job;

    // Find the child region within the job
    lower.startRow = job->startRow;
    RegionBase upper(Tsqt::RegionUpperBound);
    upper.startRow = job->endRow;

    auto k = m_regionsByStart.lower_bound(&lower);
    auto l = m_regionsByStart.upper_bound(&upper);

    while (1) {
        if (k == l)
            return nullptr;
        if (k->cref()->parent == job->id() && k->ptr->type() == type)
            return k->cref();
        ++k;
    }
}

const Region *
TermBuffer::findRegionByParent(Tsqt::RegionType type, regionid_t id) const
{
    const Region *job = safeRegion(id);

    if (job == nullptr || type == Tsqt::RegionJob)
        return job;

    // Find the child region within the job
    RegionBase lower(Tsqt::RegionLowerBound);
    lower.startRow = job->startRow;
    RegionBase upper(Tsqt::RegionUpperBound);
    upper.startRow = job->endRow;

    auto k = m_regionsByStart.lower_bound(&lower);
    auto l = m_regionsByStart.upper_bound(&upper);

    while (1) {
        if (k == l)
            return nullptr;
        if (k->cref()->parent == job->id() && k->ptr->type() == type)
            return k->cref();
        ++k;
    }
}

int
TermBuffer::recentJobs(const Region **buf, int n) const
{
    if (n == 0)
        return 0;

    int idx = 0;

    for (auto i = m_regionsByStart.rbegin(), j = m_regionsByStart.rend(); i != j; ++i)
        if (i->ptr->type() == Tsqt::RegionJob &&
            !(i->cref()->flags & (Tsq::EmptyCommand|Tsqt::IgnoredCommand)))
        {
            buf[idx++] = i->cref();
            if (idx == n)
                break;
        }

    return idx;
}

void
TermBuffer::insertSemanticRegion(Region *r)
{
    m_semantics[r->id()] = r;
    m_regionsByStart.insert(r);
    // SemanticRegion not stored in m_regionsByEnd
}

inline const Region *
TermBuffer::findOutputRegion(index_t row) const
{
    RegionBase lower(Tsqt::RegionCommand);
    lower.endRow = row;

    auto i = m_regionsByEnd.lower_bound(&lower);
    auto j = m_regionsByEnd.end();

    while (1) {
        if (i == j)
            return nullptr;
        else if (i->ptr->type() != Tsqt::RegionOutput)
            ; // do nothing
        else if (i->ptr->startRow > row)
            return nullptr;
        else if (!(i->cref()->flags & Tsq::Overwritten))
            return i->cref();

        ++i;
    }
}

bool
TermBuffer::endUpdate()
{
    if (++m_updatehi > m_size)
        m_updatehi = m_size;
    if (m_updatehi <= m_updatelo)
        return false;

    for (int i = 0; m_updatelo && m_updatelo >= m_origin &&
             m_rows[m_updatelo & m_capmask].flags & Tsq::Continuation &&
             i < MAX_CONTINUATION; --m_updatelo, ++i);
    for (int i = 0; m_updatehi < m_size &&
             m_rows[m_updatehi & m_capmask].flags & Tsq::Continuation &&
             i < MAX_CONTINUATION; ++m_updatehi, ++i);

    // Report alert content
    if (m_term->alertNeedsContent()) {
        for (index_t index = m_updatelo; index < m_updatehi; ) {
            m_str = m_rows[index & m_capmask].str;

            while (++index < m_updatehi)
            {
                const CellRow &cur = m_rows[index & m_capmask];
                if (!(cur.flags & Tsq::Continuation))
                    break;

                m_str.append(cur.str);
            }

            m_term->alert()->reportContent(m_term, m_str);
        }
    }

    // Find and remove existing link regions
    RegionBase lower(Tsqt::RegionLowerBound);
    lower.startRow = m_updatelo;

    bool rc = false;
    auto i = m_regionsByStart.lower_bound(&lower);

    while (i != m_regionsByStart.end() && i->ptr->startRow < m_updatehi) {
        if (i->ptr->type() == Tsqt::RegionLink) {
            Region *region = i->ref();
            m_semantics.erase(region->id());
            i = m_regionsByStart.erase(i);
            // LinkRegion not stored in m_regionsByEnd
            delete region;
            rc = true;
        } else {
            ++i;
        }
    }

    // Scan for osc8 links
    for (index_t index = m_updatelo; index < m_updatehi; ++index) {
        const CellRow &row = m_rows[index & m_capmask];
        const CellRange *rcur = row.ranges.data();
        const CellRange *rend = rcur + row.ranges.size();
        const Region *info;

        while (rcur < rend) {
            if (rcur->flags & Tsq::Hyperlink && (info = safeRegion(rcur->link)))
            {
                Region *r;
                r = new Region(Tsqt::RegionLink, this, m_parent->nextSemanticId());
                r->flags = Tsq::HasStart|Tsq::HasEnd|Tsqt::Updating|Tsqt::Inline;
                r->attributes = info->attributes;
                r->endRow = r->startRow = index;
                r->startCol = xByPos(row, rcur->start);

                while (rcur < rend &&
                       rcur[1].flags & Tsq::Hyperlink &&
                       rcur[1].link == rcur->link &&
                       rcur[1].start == rcur->end + 1)
                    ++rcur;

                r->endCol = xByPos(row, rcur->end + 1);

                insertSemanticRegion(r);
                rc = true;
            }
            ++rcur;
        }
    }

    // Scan for text links or pass output to custom parser
    const Region *output = nullptr;

    while (m_updatelo < m_updatehi) {
        index_t index = m_updatelo;
        bool issem = output && output->contains(index);
        unsigned pos = 0;
        const CellRow &row = m_rows[index & m_capmask];

        if (!issem) {
            output = findOutputRegion(index);
            issem = output && output->parser;
        }
        if (issem) {
            if (output->parser->populating() &&
                output->parser->setRow(index, row))
                rc = true;

            if (index != output->endRow) {
                ++m_updatelo;
                continue;
            } else if (output->endCol) {
                pos = output->parser->residualPtr(row);
            }
        }

        output = nullptr;
        m_breaks.clear();
        const std::string *str = &row.str;
        bool iscont = false;

        while (++m_updatelo < m_updatehi)
        {
            const CellRow &cur = m_rows[m_updatelo & m_capmask];
            if (!(cur.flags & Tsq::Continuation))
                break;

            if (!iscont) {
                iscont = true;
                m_str = *str;
                str = &m_str;
            }

            m_breaks.push_back(m_str.size());
            m_str.append(cur.str);
        }

        std::smatch match;
        auto i = str->begin() + pos, j = str->end();
        auto k = m_breaks.cbegin(), l = m_breaks.cend();
        unsigned base = 0;

        while (std::regex_search(i, j, match, s_urlre))
        {
            Region *r;
            r = new Region(Tsqt::RegionLink, this, m_parent->nextSemanticId());
            r->flags = Tsq::HasStart|Tsq::HasEnd|Tsqt::Updating|Tsqt::Inline;
            r->attributes[g_attr_CONTENT_URI] = QString::fromStdString(match.str());
            handleContentRegion(r);

            pos += match.position();
            i += match.position();
            while (k != l && pos >= *k) {
                ++index;
                base = *k++;
            }
            r->startRow = index;
            r->startCol = xByPtr(index, pos - base);

            pos += match.length();
            i += match.length();
            while (k != l && pos >= *k) {
                ++index;
                base = *k++;
            }
            r->endRow = index;
            r->endCol = xByPtr(index, pos - base);

            insertSemanticRegion(r);
            rc = true;
        }
    }

    return rc;
}
