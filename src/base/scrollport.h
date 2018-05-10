// Copyright © 2018 TermySequence LLC
//
// SPDX-License-Identifier: GPL-2.0-only

#pragma once

#include "viewport.h"
#include "region.h"
#include "url.h"

QT_BEGIN_NAMESPACE
class QKeyEvent;
QT_END_NAMESPACE
class TermManager;
class TermScreen;

class TermScrollport final: public TermViewport
{
    Q_OBJECT

private:
    TermManager *m_manager;
    TermScreen *m_screen;
    RegionList m_regions;
    Tsq::TermFlags m_flags;

    bool m_locked = false;
    bool m_scrolled = false;
    bool m_undersize = false;
    bool m_following = false;
    bool m_followable = false;
    bool m_searchon = false;
    bool m_scanUp = false;
    bool m_scanFetching = false;
    bool m_selecting = false;

    int m_clickPoint = 0;
    index_t m_lockedRow = INVALID_INDEX;
    index_t m_wantsRow;
    regionid_t m_wantsJob;

    regionid_t m_activePromptId = INVALID_REGION_ID;
    regionid_t m_activeJobId = INVALID_REGION_ID;
    regionid_t m_activeNoteId = INVALID_REGION_ID;
    Region m_activePrompt;
    Region m_search;
    index_t m_scanRow = INVALID_INDEX;

    QString m_searchStatus;
    TermUrl m_selectedUrl;

    QMetaObject::Connection m_mocWants;
    QMetaObject::Connection m_mocScreen;

    void setActivePrompt(const Region *region);
    void scrollToRegion(const Region *region, bool top, bool exact);

    bool tryWantsRow(index_t wantsRow);
    bool tryWantsJob(regionid_t wantsJob);

    void pushRowAttributes();
    void setFollowing(bool following);

    void updateSearchCursor();
    void updateRegions();
    void stopScan();
    void scanFetch(size_t offset, size_t size);

signals:
    void scrollChanged();
    void scrollRequest(int type);
    void selectRequest(int type, int arg);
    void contextMenuRequest();

    void flagsChanged(Tsq::TermFlags flags);
    void lockedChanged(bool locked);
    void followingChanged(bool following);
    void primarySet();

    void setTimingOriginRequest(index_t origin);
    void floatTimingOriginRequest();

    void activeJobChanged(regionid_t activeJobId);
    void searchUpdate();
    void searchStatusChanged(QString status);

    void semanticHighlight(regionid_t id);
    void selectedUrlChanged(const TermUrl &tu);

private slots:
    void handleBufferChanged();
    void handleBufferReset();
    void handleOffsetChanged();
    void handleCursorMoved(int row);

    void handleFlagsChanged(Tsq::TermFlags flags);
    void handleProcessChanged(const QString &key);
    void handleWantsRow();
    void handleWantsJob();
    void handlePopulated();

    void handleOwnershipChanged(bool ours);
    void handleAttributeChanged(const QString &key, const QString &value);
    void handleSearchChanged(bool searching);
    void handleRowFetched(index_t row);
    void removeActivePrompt(regionid_t id);

public:
    TermScrollport(TermManager *manager, TermInstance *term, QObject *parent);

    inline TermScreen* screen() { return m_screen; }
    inline TermManager* manager() { return m_manager; }
    inline const RegionList& regions() const { return m_regions; }

    inline bool locked() const { return m_locked; }
    inline bool scrolled() const { return m_scrolled; }
    inline bool following() const { return m_following; }
    inline bool followable() const { return m_followable; }
    inline bool selecting() const { return m_selecting; }

    inline Tsq::TermFlags flags() const { return m_flags; }
    inline index_t lockedRow() const { return m_lockedRow; }
    inline regionid_t activeJobId() const { return m_activeJobId; }
    regionid_t currentJobId() const;
    inline const QString& searchStatus() const { return m_searchStatus; }

    void scrollToRow(index_t row, bool exact);
    void scrollToRegion(regionid_t id, bool top, bool exact);
    void scrollToSemantic(regionid_t id);
    void scrollToRegionByClickPoint(bool top, bool exact);
    void scrollRelativeToRegion(regionid_t id, int offset);
    void scrollRelativeToSemantic(regionid_t id, int offset);
    inline void highlightSemantic() { emit semanticHighlight(INVALID_REGION_ID); }

    inline const TermUrl& selectedUrl() const { return m_selectedUrl; }
    void setSelectedUrl(const TermUrl &tu);
    void clearSelectedUrl();

    void copyRegionById(Tsqt::RegionType type, regionid_t id);
    void copyRegionByClickPoint(Tsqt::RegionType type);
    void selectRegionById(Tsqt::RegionType type, regionid_t id);
    void selectRegionByClickPoint(Tsqt::RegionType type, bool select);
    void selectPreviousLine();
    void selectNextLine();

    void annotateRegionById(Tsqt::RegionType type, regionid_t id);
    void annotateRegionByClickPoint(Tsqt::RegionType type);
    void annotateLineById(regionid_t id);
    void annotateLineByRow(int y);
    void annotateLineByClickPoint();
    void annotateSelection();

    void setActiveJob(regionid_t id);
    void setClickPoint(qreal cellHeight, int h, int y);
    void setTimingOriginByClickPoint();
    void floatTimingOrigin();

    QPoint cursor() const;
    QPoint mousePos() const;
    QPoint nearestPosition(const QPointF &pos) const;
    QPoint screenPosition(const QPointF &pos) const;

    void setShown();
    void setHidden();
    void setPrimary(bool primary);
    void setFollowable(bool followable);
    void setSelecting(bool selecting);
    void setSelectionMode(bool selectionMode);
    void setRow(index_t row, bool exact);
    bool setSize(QSize size);
    void setWants(index_t wantsRow, regionid_t wantsJob);

    void keyPressEvent(QKeyEvent *event);

    void reportPromptRequest(int type);
    void reportNoteRequest(int type);

    void startScan(bool up);
    bool scanCallback();
    void searchCallback();
    void fetchCallback();
};

#define SCROLLREQ_TOP               0
#define SCROLLREQ_BOTTOM            1
#define SCROLLREQ_PAGEUP            2
#define SCROLLREQ_PAGEDOWN          3
#define SCROLLREQ_LINEUP            4
#define SCROLLREQ_LINEDOWN          5
#define SCROLLREQ_PROMPTUP          6
#define SCROLLREQ_PROMPTDOWN        7
#define SCROLLREQ_PROMPTFIRST       8
#define SCROLLREQ_PROMPTLAST        9
#define SCROLLREQ_NOTEUP            10
#define SCROLLREQ_NOTEDOWN          11

#define SELECTREQ_HANDLE            0
#define SELECTREQ_FORWARDCHAR       1
#define SELECTREQ_BACKCHAR          2
#define SELECTREQ_FORWARDWORD       3
#define SELECTREQ_BACKWORD          4
#define SELECTREQ_DOWNLINE          5
#define SELECTREQ_UPLINE            6
