// Copyright © 2018 TermySequence LLC
//
// SPDX-License-Identifier: GPL-2.0-only

#include "common.h"
#include "app/attr.h"
#include "app/config.h"
#include "app/flags.h"
#include "scrollport.h"
#include "manager.h"
#include "term.h"
#include "buffers.h"
#include "screen.h"
#include "listener.h"
#include "scrolltimer.h"
#include "server.h"
#include "selection.h"
#include "filetracker.h"
#include "settings/global.h"
#include "settings/profile.h"
#include "settings/notedialog.h"

#include <QKeyEvent>

#define TR_TEXT1 TL("window-text", "No search is active")
#define TR_TEXT2 TL("window-text", "No match found")
#define TR_TEXT3 TL("window-text", "Fetching row %1")
#define TR_TEXT4 TL("window-text", "Match found on row %1")
#define TR_TEXT5 TL("window-text", "Scanning row %1")

TermScrollport::TermScrollport(TermManager *manager, TermInstance *term, QObject *parent):
    TermViewport(term, term->screen()->size(), parent),
    m_manager(manager),
    m_screen(term->screen()),
    m_flags(term->flags()),
    m_activePrompt(Tsqt::RegionPrompt, m_buffers, INVALID_REGION_ID),
    m_search(Tsqt::RegionSearch, m_buffers, INVALID_REGION_ID),
    m_searchStatus(TR_TEXT1)
{
    m_followable = term->isTerm() && term->profile()->followDefault();
    m_search.startCol = 0;
    g_listener->scroll()->addViewport(this);

    connect(m_buffers, SIGNAL(bufferChanged()), SLOT(handleBufferChanged()));
    connect(m_buffers, SIGNAL(bufferReset()), SLOT(handleBufferReset()));
    connect(m_buffers, SIGNAL(fetchScanRow(index_t)), SLOT(handleRowFetched(index_t)));
    connect(m_buffers, SIGNAL(promptDeleted(regionid_t)), SLOT(removeActivePrompt(regionid_t)));
    connect(m_term, SIGNAL(flagsChanged(Tsq::TermFlags)), SLOT(handleFlagsChanged(Tsq::TermFlags)));
    connect(m_term, SIGNAL(processChanged(const QString&)), SLOT(handleProcessChanged(const QString&)));
    connect(m_term, SIGNAL(localSizeChanged(QSize)), SLOT(handleBufferChanged()));
    connect(m_term, SIGNAL(ownershipChanged(bool)), SLOT(handleOwnershipChanged(bool)));
    connect(m_term, SIGNAL(attributeChanged(QString,QString)), SLOT(handleAttributeChanged(const QString&,const QString&)));
    connect(m_term, SIGNAL(searchChanged(bool)), SLOT(handleSearchChanged(bool)));
    connect(m_term->files(), &FileTracker::directoryChanged, this, &TermScrollport::clearSelectedUrl);

    connect(this, SIGNAL(offsetChanged(int)), SLOT(handleOffsetChanged()));
    connect(this, SIGNAL(inputReceived()), m_screen, SIGNAL(inputReceived()));

    moveToEnd();
    handleOwnershipChanged(term->ours());
}

void
TermScrollport::pushRowAttributes()
{
    if (m_primary && m_term->ours()) {
        AttributeMap map;
        QString row, job;
        if (m_locked)
            row = QString::number(m_lockedRow);
        if (m_activeJobId != INVALID_REGION_ID)
            job = QString::number(m_activeJobId);

        if (m_term->attributes().value(g_attr_PREF_ROW) != row)
            map[g_attr_PREF_ROW] = row;
        if (m_term->attributes().value(g_attr_PREF_JOB) != job)
            map[g_attr_PREF_JOB] = job;

        if (!map.isEmpty())
            g_listener->pushTermAttributes(m_term, map);
    }
}

void
TermScrollport::handleBufferChanged()
{
    if (!m_locked)
        moveToEnd();
    else
        moveToRow(&m_lockedRow, true);
}

void
TermScrollport::handleBufferReset()
{
    m_locked = false;
    m_lockedRow = INVALID_INDEX;
    moveToEnd();
    emit lockedChanged(false);
}

void
TermScrollport::handleOffsetChanged()
{
    m_scrolled = (m_offset < m_screen->offset()) &&
        (m_offset + height() < m_buffers->size());

    g_listener->scroll()->setFetchTimer(this);
}

void
TermScrollport::handleCursorMoved(int row)
{
    if (!m_locked)
        moveToOffset(row);
}

void
TermScrollport::handleFlagsChanged(Tsq::TermFlags flags)
{
    m_flags = (flags & ~Tsqt::SelectMode) | (m_flags & Tsqt::SelectMode);
    emit flagsChanged(m_flags);
}

void
TermScrollport::handleProcessChanged(const QString &key)
{
    if (m_primary && key == g_attr_PROC_CWD)
        m_term->server()->setCwd(m_term->process()->workingDir);
}

void
TermScrollport::handleAttributeChanged(const QString &key, const QString &value)
{
    if (m_following) {
        if (key == g_attr_PREF_ROW) {
            bool ok;
            index_t row = value.toULongLong(&ok);
            scrollToRow(ok ? row : INVALID_INDEX, true);
        }
        else if (key == g_attr_PREF_JOB) {
            const Region *job = m_buffers->safeRegion(value.toUInt());
            if (job)
                setActiveJob(job->id());
            else if (m_activePromptId != INVALID_REGION_ID)
                removeActivePrompt(m_activePromptId);
        }
    }
}

void
TermScrollport::handleOwnershipChanged(bool ours)
{
    bool following = false;

    if (ours) {
        pushRowAttributes();
    } else if (m_followable) {
        following = true;
    }

    setFollowing(following);
}

void
TermScrollport::setFollowing(bool following)
{
    if (m_following != following) {
        if ((m_following = following)) {
            const auto &a = m_term->attributes();
            handleAttributeChanged(g_attr_PREF_ROW, a.value(g_attr_PREF_ROW));
            handleAttributeChanged(g_attr_PREF_JOB, a.value(g_attr_PREF_JOB));
        }
        emit followingChanged(following);
    }
}

void
TermScrollport::setFollowable(bool followable)
{
    if (m_followable != followable && m_term->isTerm()) {
        m_followable = followable;
        setFollowing(m_followable && !m_term->ours());
    }
}

void
TermScrollport::setSelectionMode(bool selectionMode)
{
    auto flags = m_flags;

    if (selectionMode && m_selecting)
        flags |= Tsqt::SelectMode;
    else
        flags &= ~Tsqt::SelectMode;

    if (m_flags != flags)
        emit flagsChanged(m_flags = flags);
}

void
TermScrollport::setSelecting(bool selecting)
{
    if (!(m_selecting = selecting))
        setSelectionMode(false);
    else if (g_global->autoSelect())
        setSelectionMode(true);
}

void
TermScrollport::setShown()
{
    if (m_activePromptId != INVALID_REGION_ID)
        m_buffers->activateRegion(&m_activePrompt);
    if (m_search.startRow != INVALID_INDEX)
        m_buffers->activateRegion(&m_search);
}

void
TermScrollport::setHidden()
{
    if (m_activePromptId != INVALID_REGION_ID)
        m_buffers->deactivateRegion(&m_activePrompt);
    if (m_search.startRow != INVALID_INDEX)
        m_buffers->deactivateRegion(&m_search);
}

void
TermScrollport::setPrimary(bool primary)
{
    if (m_primary != primary) {
        m_primary = primary;
        m_term->clearAlert();

        if (primary) {
            m_term->setLocalSize(size());
            m_term->server()->setCwd(m_term->process()->workingDir);
            pushRowAttributes();
            emit primarySet();
        }
    }
}

void
TermScrollport::setRow(index_t row, bool exact)
{
    if (m_lockedRow != row) {
        m_lockedRow = row;
        bool locked;

        if (row != INVALID_INDEX) {
            locked = true;
            moveToRow(&m_lockedRow, exact);
        } else {
            locked = false;
            moveToEnd();
        }

        if (m_locked != locked)
            emit lockedChanged(m_locked = locked);

        pushRowAttributes();
    }
}

bool
TermScrollport::setSize(QSize size)
{
    bool changed = false;
    bool undersize = size.height() < m_screen->height();

    if (m_bounds != size) {
        m_bounds = size;
        changed = true;
    }

    if (m_undersize != undersize) {
        if ((m_undersize = undersize))
            m_mocScreen = connect(m_screen, SIGNAL(cursorMoved(int)),
                                  SLOT(handleCursorMoved(int)));
        else
            disconnect(m_mocScreen);
    }

    if (m_primary) {
        changed |= m_term->setLocalSize(size);
    }
    else if (!m_locked) {
        if (m_undersize)
            moveToOffset(m_screen->row());
        else
            moveToEnd();
    }

    emit scrollChanged();
    return changed;
}

void
TermScrollport::setClickPoint(qreal cellHeight, int h, int y)
{
    h /= cellHeight;
    y /= cellHeight;

    if (y < 0)
        y = 0;
    if (y > h - 1)
        y = h - 1;

    m_clickPoint = h - y;
}

void
TermScrollport::startScan(bool up)
{
    if (m_term->searching())
    {
        m_scanUp = up;
        m_scanFetching = false;

        if (m_search.startRow != INVALID_INDEX)
            m_scanRow = up ? m_search.startRow - 1 : m_search.startRow + 1;
        else if (up)
            m_scanRow = m_buffers->origin() + m_offset + height() - 1;
        else
            m_scanRow = m_buffers->origin() + m_offset;

        g_listener->scroll()->setScanTimer(this);
    }
}

inline void
TermScrollport::stopScan()
{
    m_scanRow = INVALID_INDEX;
    m_scanFetching = false;
}

inline void
TermScrollport::scanFetch(size_t offset, size_t size)
{
    // Try to fetch multiple rows at a time
    size_t lower = offset, upper = offset;

    if (m_scanUp) {
        ++upper;
        for (int i = 0; i < SCAN_PREFETCH; ++i) {
            if (lower == 0l)
                break;
            if (m_buffers->row(lower - 1).flags & Tsqt::Downloaded)
                break;
            --lower;
        }
    } else {
        for (int i = 0; i < SCAN_PREFETCH; ++i) {
            if (++upper == size)
                break;
            if (m_buffers->row(upper).flags & Tsqt::Downloaded)
                break;
        }
    }

    m_buffers->fetchRows(lower, upper);
}

bool
TermScrollport::scanCallback()
{
    // Is the current row in bounds?
    index_t origin = m_buffers->origin();
    size_t size = m_buffers->size();

    if (m_scanRow < origin || m_scanRow >= origin + size) {
        emit searchStatusChanged(m_searchStatus = TR_TEXT2);
        stopScan();
        return false;
    }

    // Has the current row been fetched?
    size_t offset = m_scanRow - origin;

    if ((m_buffers->row(offset).flags & Tsqt::Downloaded) == 0) {
        // No - schedule a fetch
        scanFetch(offset, size);
        m_scanFetching = true;
        emit searchStatusChanged(m_searchStatus = TR_TEXT3.arg(m_scanRow));
        return false;
    }
    else if (m_scanRow == m_buffers->searchRows(offset, offset + 1)) {
        // Match - stop here
        m_search.startRow = m_search.endRow = m_scanRow;
        scrollToRow(m_scanRow, false);
        m_buffers->activateRegion(&m_search);
        emit searchStatusChanged(m_searchStatus = TR_TEXT4.arg(m_scanRow));
        stopScan();
        return false;
    }
    else if (m_scanUp) {
        // No match - keep searching
        --m_scanRow;
    }
    else {
        // No match - keep searching
        ++m_scanRow;
    }

    if ((m_scanRow % 100) == 0) {
        emit searchStatusChanged(m_searchStatus = TR_TEXT5.arg(m_scanRow));
    }
    return true;
}

void
TermScrollport::handleRowFetched(index_t row)
{
    if (m_scanFetching && m_scanRow == row) {
        m_scanFetching = false;
        g_listener->scroll()->setScanTimer(this);
    }
}

void
TermScrollport::handleSearchChanged(bool)
{
    m_search.startRow = m_search.endRow = INVALID_INDEX;
    m_buffers->deactivateRegion(&m_search);
    g_listener->scroll()->setSearchTimer(this);

    g_listener->scroll()->unsetScanTimer(this);
    emit searchStatusChanged(m_searchStatus = TR_TEXT1);
    stopScan();
}

inline void
TermScrollport::updateSearchCursor()
{
    index_t row = m_buffers->origin() + m_offset;

    bool searchon = (m_search.startRow >= row &&
                     m_search.startRow < row + height() &&
                     !m_term->overlayActive());

    if (m_searchon != searchon)
    {
        if ((m_searchon = searchon))
            m_regions.list.push_back(&m_search);
        else
            m_regions.list.pop_back();
    }
}

void
TermScrollport::searchCallback()
{
    size_t start = m_offset;
    size_t end = m_offset + height();

    index_t row = m_buffers->searchRows(start, end);

    if (row != INVALID_INDEX && m_search.startRow == INVALID_INDEX) {
        m_search.startRow = m_search.endRow = row;
        m_buffers->activateRegion(&m_search);
        updateSearchCursor();
    }

    emit searchUpdate();
}

void
TermScrollport::fetchCallback()
{
    // fetch unseen rows if necessary
    if (m_offset < m_screen->offset())
    {
        size_t start = m_offset;
        size_t end = m_offset + height();

        if (end > m_screen->offset())
            end = m_screen->offset();

        m_buffers->fetchRows(start, end);
    }

    // run search if necessary
    if (m_term->searching())
        searchCallback();
}

QPoint
TermScrollport::cursor() const
{
    QPoint cursor = m_screen->cursor();
    cursor.ry() += (int)(m_screen->offset() - m_offset);
    return cursor;
}

QPoint
TermScrollport::mousePos() const
{
    QPoint mousePos = m_term->mousePos();
    mousePos.ry() += (int)(m_screen->offset() - m_offset);
    return mousePos;
}

QPoint
TermScrollport::nearestPosition(const QPointF &pos) const
{
    int x = 0, y = (int)pos.y() + m_offset;
    size_t size = m_buffers->size();
    // NOTE size_t collides with int here

    if (size == 0)
        goto out;
    if (y >= size)
        y = size - 1;

    x = m_buffers->posByX(y, pos.x() + 0.5);
out:
    return QPoint(x, y);
}

QPoint
TermScrollport::screenPosition(const QPointF &pos) const
{
    int x = (int)pos.x();
    int y = (int)pos.y() + m_offset - m_screen->offset();

    if (m_scrolled || y < 0 || y >= m_screen->height() || x >= m_screen->width())
        return QPoint(-1, -1);
    else
        return QPoint(x, y);
}

void
TermScrollport::keyPressEvent(QKeyEvent *event)
{
    if (m_term->keyPressEvent(m_manager, event, m_flags)) {
        emit inputReceived();
    }
}

void
TermScrollport::removeActivePrompt(regionid_t id)
{
    if (m_activePromptId == id) {
        m_buffers->deactivateRegion(&m_activePrompt);
        m_activePromptId = INVALID_REGION_ID;
        m_activeJobId = INVALID_REGION_ID;
        emit activeJobChanged(m_activeJobId);
        pushRowAttributes();
    }
}

void
TermScrollport::setActivePrompt(const Region *region)
{
    // non-null region expected
    if (m_activePromptId != region->id()) {
        m_activePrompt = *region;
        m_activePromptId = region->id();
        m_activeJobId = region->parent;
        m_buffers->activateRegion(&m_activePrompt);
        emit activeJobChanged(m_activeJobId);
        pushRowAttributes();
    }
}

void
TermScrollport::setActiveJob(regionid_t id)
{
    const auto *region = m_buffers->findRegionByParent(Tsqt::RegionPrompt, id);
    if (region)
        setActivePrompt(region);
}

regionid_t
TermScrollport::currentJobId() const
{
    index_t start = m_buffers->origin() + m_offset;
    index_t end = start + height();

    while (end-- > start) {
        const auto *job = m_buffers->findRegionByRow(Tsqt::RegionJob, end);
        if ((job->flags & (Tsq::HasCommand|Tsq::EmptyCommand)) == Tsq::HasCommand)
            return job->id();
    }

    return INVALID_REGION_ID;
}

void
TermScrollport::scrollToRow(index_t row, bool exact)
{
    setRow(row, exact);
    emit scrollChanged();
}

void
TermScrollport::scrollToRegion(const Region *region, bool top, bool exact)
{
    // non-null region expected
    index_t row;

    if (top) {
        row = region->startRow;
    } else {
        row = m_buffers->origin() + height() - 1;
        if (row < region->endRow)
            row = region->endRow;

        row -= height() - 1;
    }

    scrollToRow(row, exact);
}

void
TermScrollport::scrollToRegion(regionid_t id, bool top, bool exact)
{
    const Region *region = m_buffers->safeRegion(id);
    if (region) {
        // Try to set active prompt
        switch (region->type()) {
        case Tsqt::RegionJob:
            setActiveJob(id);
            break;
        case Tsqt::RegionPrompt:
            setActivePrompt(region);
            break;
        case Tsqt::RegionCommand:
        case Tsqt::RegionOutput:
            setActiveJob(region->parent);
            break;
        default:
            break;
        }

        scrollToRegion(region, top, exact);
    }
}

void
TermScrollport::scrollToRegionByClickPoint(bool top, bool exact)
{
    index_t row = m_buffers->origin() + m_offset + height() - m_clickPoint;
    const Region *region = m_buffers->findRegionByRow(Tsqt::RegionJob, row);

    if (region) {
        setActiveJob(region->id());
        scrollToRegion(region, top, exact);
    }
}

void
TermScrollport::scrollRelativeToRegion(regionid_t id, int offset)
{
    const Region *region = m_buffers->safeRegion(id);
    if (region)
        scrollToRow(region->startRow + offset, true);
}

void
TermScrollport::scrollToSemantic(regionid_t id)
{
    const Region *region = m_buffers->safeSemantic(id);
    if (region) {
        scrollToRegion(region, true, false);
        emit semanticHighlight(id);
    }
}

void
TermScrollport::scrollRelativeToSemantic(regionid_t id, int offset)
{
    const Region *region = m_buffers->safeSemantic(id);
    if (region) {
        scrollToRow(region->startRow + offset, true);
        emit semanticHighlight(id);
    }
}

void
TermScrollport::reportPromptRequest(int type)
{
    const Region *region = &m_activePrompt;
    index_t start = m_buffers->origin() + m_offset;
    index_t end = start + height();

    switch (type) {
    case SCROLLREQ_PROMPTUP:
        region = (m_activePromptId == INVALID_REGION_ID ||
                  region->startRow >= end || region->endRow < start) ?
            m_buffers->upRegion(Tsqt::RegionPrompt, end) :
            m_buffers->prevRegion(Tsqt::RegionPrompt, m_activePromptId);
        break;
    case SCROLLREQ_PROMPTDOWN:
        region = (m_activePromptId == INVALID_REGION_ID ||
                  region->startRow >= end || region->endRow < start) ?
            m_buffers->downRegion(Tsqt::RegionPrompt, start) :
            m_buffers->nextRegion(Tsqt::RegionPrompt, m_activePromptId);
        break;
    case SCROLLREQ_PROMPTFIRST:
        region = m_buffers->firstRegion(Tsqt::RegionPrompt);
        break;
    case SCROLLREQ_PROMPTLAST:
        region = m_buffers->lastRegion(Tsqt::RegionPrompt);
        break;
    }

    if (region) {
        setActivePrompt(region);
        scrollToRegion(region, true, false);
    }
}

void
TermScrollport::reportNoteRequest(int type)
{
    const Region *region = m_buffers->safeRegion(m_activeNoteId);
    index_t start = m_buffers->origin() + m_offset;
    index_t end = start + height();

    switch (type) {
    case SCROLLREQ_NOTEUP:
        region = (region && (region->startRow >= end || region->endRow < start)) ?
            m_buffers->upRegion(Tsqt::RegionUser, end) :
            m_buffers->prevRegion(Tsqt::RegionUser, m_activeNoteId);
        break;
    case SCROLLREQ_NOTEDOWN:
        region = (region && (region->startRow >= end || region->endRow < start)) ?
            m_buffers->downRegion(Tsqt::RegionUser, start) :
            m_buffers->nextRegion(Tsqt::RegionUser, m_activeNoteId);
        break;
    }

    if (region) {
        m_activeNoteId = region->id();
        scrollToRegion(region, true, false);
    }
}

void
TermScrollport::copyRegionById(Tsqt::RegionType type, regionid_t id)
{
    const Region *region;

    if (id != INVALID_REGION_ID) {
        region = m_buffers->findRegionByParent(type, id);
    } else if (m_activeJobId != INVALID_REGION_ID) {
        region = m_buffers->findRegionByParent(type, m_activeJobId);
    } else {
        const Region *tmp = m_buffers->modtimeRegion();
        if (tmp) {
            id = (tmp->type() == Tsqt::RegionJob) ? tmp->id() : tmp->parent;
            region = m_buffers->findRegionByParent(type, id);
        } else {
            region = nullptr;
        }
    }

    if (region)
        m_manager->reportClipboardCopy(region->clipboardCopy());
}

void
TermScrollport::selectRegionById(Tsqt::RegionType type, regionid_t id)
{
    const Region *region, *job;

    if (id != INVALID_REGION_ID) {
        region = m_buffers->findRegionByParent(type, id);
        job = m_buffers->safeRegion(id);
    } else if (m_activeJobId != INVALID_REGION_ID) {
        region = m_buffers->findRegionByParent(type, m_activeJobId);
        job = m_buffers->safeRegion(m_activeJobId);
    } else {
        const Region *tmp = m_buffers->modtimeRegion();
        if (tmp) {
            id = (tmp->type() == Tsqt::RegionJob) ? tmp->id() : tmp->parent;
            region = m_buffers->findRegionByParent(type, id);
            job = m_buffers->safeRegion(id);
        } else {
            region = nullptr;
        }
    }

    if (region) {
        if (job) {
            setActiveJob(job->id());
            scrollToRegion(job, true, true);
        }
        m_buffers->selection()->selectRegion(region);
    }
}

void
TermScrollport::copyRegionByClickPoint(Tsqt::RegionType type)
{
    index_t row = m_buffers->origin() + m_offset + height() - m_clickPoint;
    const Region *region = m_buffers->findRegionByRow(type, row);

    if (region)
        m_manager->reportClipboardCopy(region->clipboardCopy());
}

void
TermScrollport::selectRegionByClickPoint(Tsqt::RegionType type, bool select)
{
    index_t row = m_buffers->origin() + m_offset + height() - m_clickPoint;
    const Region *region = m_buffers->findRegionByRow(type, row);

    if (region) {
        const Region *job;

        if (region->type() == Tsqt::RegionJob)
            job = region;
        else
            job = m_buffers->safeRegion(region->parent);

        if (job) {
            setActiveJob(job->id());
            scrollToRegion(job, true, true);
        }
        if (select) {
            m_buffers->selection()->selectRegion(region);
        }
    }
}

void
TermScrollport::annotateRegionById(Tsqt::RegionType type, regionid_t id)
{
    const Region *region;

    if (id != INVALID_REGION_ID) {
        region = m_buffers->findRegionByParent(type, id);
    } else if (m_activeJobId != INVALID_REGION_ID) {
        region = m_buffers->findRegionByParent(type, m_activeJobId);
    } else {
        const Region *tmp = m_buffers->modtimeRegion();
        if (tmp) {
            id = (tmp->type() == Tsqt::RegionJob) ? tmp->id() : tmp->parent;
            region = m_buffers->findRegionByParent(type, id);
        } else {
            region = nullptr;
        }
    }

    if (region)
        (new NoteDialog(m_term, region, m_manager->parentWidget()))->show();
}

void
TermScrollport::annotateRegionByClickPoint(Tsqt::RegionType type)
{
    index_t row = m_buffers->origin() + m_offset + height() - m_clickPoint;
    const Region *region = m_buffers->findRegionByRow(type, row);

    if (region)
        (new NoteDialog(m_term, region, m_manager->parentWidget()))->show();
}

void
TermScrollport::annotateLineById(regionid_t id)
{
    RegionBase tmp(Tsqt::RegionUser);

    if (id == INVALID_REGION_ID) {
        index_t row = m_buffers->origin() + m_offset + m_screen->cursor().y();
        tmp.startRow = tmp.endRow = row;
        tmp.startCol = 0;
        (new NoteDialog(m_term, &tmp, m_manager->parentWidget()))->show();
    } else {
        const Region *region = m_buffers->safeRegion(id);

        if (region && region->flags & Tsq::HasStart) {
            tmp.startRow = tmp.endRow = region->startRow;
            tmp.startCol = 0;
            (new NoteDialog(m_term, &tmp, m_manager->parentWidget()))->show();
        }
    }
}

void
TermScrollport::annotateLineByRow(int y)
{
    size_t row = m_offset;
    row += (y < 0) ? height() + y : y;

    if (row > m_offset + cursor().y())
        row = m_offset + cursor().y();
    if (row >= m_buffers->size0())
        row = m_buffers->size0() - 1;

    RegionBase tmp(Tsqt::RegionUser);
    tmp.startRow = tmp.endRow = m_buffers->origin() + row;
    tmp.startCol = 0;
    (new NoteDialog(m_term, &tmp, m_manager->parentWidget()))->show();
}

void
TermScrollport::annotateLineByClickPoint()
{
    if (m_clickPoint > height())
        m_clickPoint = height();

    index_t row = m_offset + height() - m_clickPoint;

    if (row >= m_buffers->size0())
        row = m_buffers->size0() - 1;

    RegionBase tmp(Tsqt::RegionUser);
    tmp.startRow = tmp.endRow = m_buffers->origin() + row;
    tmp.startCol = 0;
    (new NoteDialog(m_term, &tmp, m_manager->parentWidget()))->show();
}

void
TermScrollport::annotateSelection()
{
    auto *selection = m_buffers->selection();

    if (selection->isEmpty()) {
        annotateLineByClickPoint();
    } else {
        (new NoteDialog(m_term, selection, m_manager->parentWidget()))->show();
    }
}

void
TermScrollport::selectPreviousLine()
{
    auto *selection = m_buffers->selection();
    size_t y = m_selecting ?
        selection->startRow - m_buffers->origin() :
        m_offset + height() - 1;

    for (int i = 0; y > 0 && i < MAX_SELECTION_LINES; ++i)
        if (!m_buffers->safeRow(--y).str.empty()) {
            selection->selectLineAt(QPoint(0, y));
            scrollToRow(y + m_buffers->origin(), false);
            break;
        }
}

void
TermScrollport::selectNextLine()
{
    auto *selection = m_buffers->selection();
    size_t y = m_selecting ?
        selection->startRow - m_buffers->origin() :
        m_offset;

    for (int i = 0; y < m_buffers->size() - 1 && i < MAX_SELECTION_LINES; ++i)
        if (!m_buffers->safeRow(++y).str.empty()) {
            selection->selectLineAt(QPoint(0, y));
            scrollToRow(y + m_buffers->origin(), false);
            break;
        }
}

void
TermScrollport::setTimingOriginByClickPoint()
{
    index_t row = m_buffers->origin() + m_offset + height() - m_clickPoint;

    emit setTimingOriginRequest(row);
}

void
TermScrollport::floatTimingOrigin()
{
    emit floatTimingOriginRequest();
}

bool
TermScrollport::tryWantsRow(index_t wantsRow)
{
    index_t origin = m_buffers->origin();
    size_t size = m_buffers->size();

    // Determine if we can scroll to the wanted offset
    if (wantsRow >= origin && wantsRow < origin + size) {
        scrollToRow(wantsRow, true);
        return true;
    }

    return false;
}

bool
TermScrollport::tryWantsJob(regionid_t wantsJob)
{
    // Determine if the requested job exists
    const Region *region = m_buffers->safeRegion(wantsJob);
    if (region && region->type() == Tsqt::RegionJob) {
        setActiveJob(region->id());
        return true;
    }

    return false;
}

void
TermScrollport::handleWantsRow()
{
    disconnect(m_mocWants);
    tryWantsRow(m_wantsRow);
}

void
TermScrollport::handlePopulated()
{
    disconnect(m_manager, SIGNAL(populated()), this, SLOT(handlePopulated()));
    disconnect(m_buffers, SIGNAL(regionChanged()), this, SLOT(handleWantsJob()));
}

void
TermScrollport::handleWantsJob()
{
    if (tryWantsJob(m_wantsJob))
        handlePopulated();
}

void
TermScrollport::setWants(index_t wantsRow, regionid_t wantsJob)
{
    if (wantsRow != INVALID_INDEX && !tryWantsRow(wantsRow) && m_manager->populating())
    {
        m_wantsRow = wantsRow;
        m_mocWants = connect(m_buffers, SIGNAL(bufferChanged()), SLOT(handleWantsRow()));
    }
    if (wantsJob != INVALID_REGION_ID && !tryWantsJob(wantsJob) && m_manager->populating())
    {
        m_wantsJob = wantsJob;
        connect(m_buffers, SIGNAL(regionChanged()), SLOT(handleWantsJob()));
        connect(m_manager, SIGNAL(populated()), SLOT(handlePopulated()));
    }
}

void
TermScrollport::setSelectedUrl(const TermUrl &tu)
{
    if (m_selectedUrl != tu) {
        emit selectedUrlChanged(m_selectedUrl = tu);
    }
}

void
TermScrollport::clearSelectedUrl()
{
    if (!m_selectedUrl.isEmpty()) {
        m_selectedUrl.clear();
        emit selectedUrlChanged(m_selectedUrl);
    }
}

void
TermScrollport::updateRegions()
{
    m_buffers->updateRows(m_offset, m_offset + height(), &m_regions);

    // updateSearchCursor
    index_t row = m_buffers->origin() + m_offset;

    if ((m_searchon = (m_search.startRow >= row &&
                       m_search.startRow < row + height() &&
                       !m_term->overlayActive())))
    {
        m_regions.list.push_back(&m_search);
    }
}
