// Copyright © 2018 TermySequence LLC
//
// SPDX-License-Identifier: GPL-2.0-only

#pragma once

#include "buffer.h"
#include "overlay.h"

#include <QObject>

class TermBuffers final: public QObject, public BufferBase
{
    Q_OBJECT

private:
    TermBuffer *m_buffers;
    const TermOverlay *m_overlay = nullptr;

    size_t m_size;

    int m_activeBuffer = 0;
    unsigned m_regionState = 0;
    bool m_regionChanged = false;
    bool m_selectionActive = false;

    regionid_t m_nextSemanticId = 0;

    // controlled by FetchTimer
    index_t m_fetchPos = 0;
    index_t m_fetchNext = INVALID_INDEX;

    void recalculateSizes();

signals:
    void contentChanged();
    void bufferChanged();
    void bufferReset();
    void regionChanged();

    void fetchScanRow(index_t row);
    void fetchFetchRow();
    void fetchPosChanged();

    void jobChanged(TermInstance *term, Region *region);
    void noteChanged(TermInstance *term, Region *region);
    void noteDeleted(TermInstance *term, Region *region);
    void promptDeleted(regionid_t id);

public:
    TermBuffers(TermInstance *term);
    ~TermBuffers();

    inline size_t size() const { return m_overlay ? m_overlay->size() : m_size; }
    inline size_t size0() const { return m_overlay ? m_overlay->size() : m_buffers->size(); }
    inline size_t offset() const { return 0; }
    inline index_t origin() const { return m_buffers->origin(); }
    inline index_t last() const { return m_buffers->origin() + size(); }
    inline unsigned regionState() const { return m_regionState; }
    inline uint8_t caporder() const { return m_buffers->caporder(); }
    inline bool noScrollback() const { return m_buffers->noScrollback(); }
    inline index_t fetchPos() const { return m_fetchPos; }
    inline index_t fetchNext() const { return m_fetchNext; }
    inline TermBuffer* buffer0() { return m_buffers; }
    inline bool selectionActive() const { return m_selectionActive; }

    TermBuffer* bufferAt(size_t idx);
    void setActiveBuffer(uint8_t bufid);
    void beginSetOverlay(const TermOverlay *overlay);
    void endSetOverlay();

    const CellRow& row(size_t i) const;
    const CellRow& safeRow(size_t i) const;
    void setRow(index_t i, const uint32_t *data, const char *str, unsigned len, bool pushed);
    void setRegion(const uint32_t *data, AttributeMap &attributes);

    void updateRows(size_t start, size_t end, RegionList *regionret);
    index_t searchRows(size_t start, size_t end);
    void fetchRows(size_t start, size_t end);

    Cell cellByPos(size_t idx, column_t pos) const;
    Cell cellByX(size_t idx, int x) const;
    column_t posByX(size_t idx, int x) const;
    column_t xByPos(index_t idx, column_t pos) const;
    column_t xSize(index_t idx) const;

    void changeCapacity(uint8_t bufid, index_t rows, uint8_t capspec);
    void changeLength(uint8_t bufid, index_t rows);

    regionid_t nextSemanticId();
    regionid_t peekSemanticId() const;

    inline const Region* safeRegion(regionid_t id) const
    { return m_buffers->safeRegion(id); }
    inline const Region* safeSemantic(regionid_t id) const
    { return m_buffers->safeSemantic(id); }

    inline const Region* firstRegion(Tsqt::RegionType type) const
    { return m_buffers->firstRegion(type); }
    inline const Region* lastRegion(Tsqt::RegionType type) const
    { return m_buffers->lastRegion(type); }
    inline const Region* nextRegion(Tsqt::RegionType type, regionid_t id) const
    { return m_buffers->nextRegion(type, id); }
    inline const Region* prevRegion(Tsqt::RegionType type, regionid_t id) const
    { return m_buffers->prevRegion(type, id); }
    inline const Region* downRegion(Tsqt::RegionType type, index_t row) const
    { return m_buffers->downRegion(type, row); }
    inline const Region* upRegion(Tsqt::RegionType type, index_t row) const
    { return m_buffers->upRegion(type, row); }

    inline const Region* modtimeRegion() const
    { return m_buffers->modtimeRegion(); }
    inline const Region* findRegionByRow(Tsqt::RegionType type, index_t row) const
    { return m_buffers->findRegionByRow(type, row); }
    inline const Region* findRegionByParent(Tsqt::RegionType type, regionid_t id) const
    { return m_buffers->findRegionByParent(type, id); }
    inline const auto& activeRegions() const
    { return m_buffers->activeRegions(); }

    inline int recentJobs(const Region **buf, int n) const
    { return m_buffers->recentJobs(buf, n); }

    void activateRegion(const Region *region);
    void deactivateRegion(const Region *region);
    void activateSelection();
    void deactivateSelection();

    void beginUpdate();
    void endUpdate();
    void reportRegionChanged();

    inline void reportJobChanged(Region *region)
    { emit jobChanged(m_term, region); }
    inline void reportNoteChanged(Region *region)
    { emit noteChanged(m_term, region); }
    inline void reportNoteDeleted(Region *region)
    { emit noteDeleted(m_term, region); }
    inline void reportPromptDeleted(regionid_t id)
    { emit promptDeleted(id); }
    inline void reportOverlayChanged()
    { emit contentChanged(); }

    inline void setFetchPos(index_t fetchPos)
    { m_fetchPos = fetchPos; emit fetchPosChanged(); }
    inline void setFetchNext(index_t fetchNext)
    { m_fetchNext = fetchNext; }
};

inline void
TermBuffers::reportRegionChanged()
{
    ++m_regionState;
    emit regionChanged();
    emit contentChanged();
}

inline regionid_t
TermBuffers::nextSemanticId()
{
    ++m_nextSemanticId;
    return m_nextSemanticId += (m_nextSemanticId == INVALID_REGION_ID);
}

inline regionid_t
TermBuffers::peekSemanticId() const
{
    return m_nextSemanticId + 1 + (m_nextSemanticId + 1 == INVALID_REGION_ID);
}
