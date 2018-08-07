// Copyright © 2018 TermySequence LLC
//
// SPDX-License-Identifier: GPL-2.0-only

#pragma once

#include "conn.h"
#include "cell.h"
#include "rect.h"
#include "codestring.h"

#include <unordered_set>

namespace Tsq { class ProtocolMarshaler; }
class TermEmulator;
class TermStatusTracker;
class TermFilemon;
class Translator;
class Region;
struct PtyParams;
struct EmulatorParams;

class TermInstance final: public ConnInstance
{
private:
    TermEmulator *m_emulator;
    TermStatusTracker *m_status;
    bool m_haveOutcome = true, m_haveClosed = true;

    int32_t m_modTime;
    int32_t m_rateStatus = 0;
    int32_t m_rateIdleTime, m_ratePushTime;
    int64_t m_baseTime;
    int64_t m_launchTime = 0;

    TermFilemon *m_filemon;
    const Translator *m_translator;
    PtyParams *m_params;

    std::unordered_set<Region*> m_incomingRegions;

    void threadMain();
    bool handleFd();
    bool handleWork(const WorkItem &item);
    bool handleIdle();

    void resetRatelimit();
    void pushChanges(bool activate = true);

    void handleTermEvent(char *buf, unsigned len, bool running);
    void handleTermReset(const char *buf, unsigned len, Tsq::ResetFlags arg);
    void handleTermResize(Size size);
    void handleBufferResize(uint8_t bufid, uint8_t caporder);
    void handleUpdateEnviron(SharedStringMap *environ);
    void handleMouseMove(Point mousePos);
    void handleCreateRegion(Region *region);
    void handleRemoveRegion(regionid_t id, uint8_t bufid);
    void handleScrollLock(bool hard, bool enabled);

    void launch();
    void handleStatusAttributes(StringMap &map);
    void handleProcessExited(int disposition);
    void halt();

    void configuredInitParams(EmulatorParams *params) const;
    std::string configuredStartParams(PtyParams *params) const;
    int configuredExitAction() const;
    std::pair<int,int> configuredAutoClose() const;

    void reportAttributeChange(const std::string &key, const std::string &value);
    void postConnect();
    void postDisconnect(bool locked);

private:
    void setup(const Tsq::Uuid &id, const Tsq::Uuid &owner, Size size,
               OwnershipInfo *oi, EmulatorParams *params);
    TermInstance(const Tsq::Uuid &id, const Tsq::Uuid &owner,
                 Size size, OwnershipInfo *oi,
                 const TermInstance *copyfrom);

public:
    TermInstance(const Tsq::Uuid &id, const Tsq::Uuid &owner,
                 Size size, OwnershipInfo *oi);
    ~TermInstance();

    inline TermEmulator* emulator() { return m_emulator; }
    inline TermFilemon* filemon() { return m_filemon; }
    inline const Translator* translator() const { return m_translator; }
    inline const int32_t* modTimePtr() const { return &m_modTime; }

    TermInstance* commandDuplicate(const Tsq::Uuid &id, const Tsq::Uuid &owner,
                                   Size size, OwnershipInfo *oi);
    void commandGetRows(uint8_t bufid, index_t start, index_t end,
                        std::vector<CellRow> &rows,
                        std::vector<Region> &regions);
    bool commandGetContent(contentid_t id, Tsq::ProtocolMarshaler *m);
    bool commandGetRegion(uint8_t bufid, regionid_t id, Region &region);
    void commandCreateRegion(Region *region);

    bool reportHardScrollLock(bool enabled, bool query);
    void reportDirectoryUpdate(const std::string &msg);
    void reportFileUpdate(const std::string &name, const std::string &msg);
    void reportFileUpdates(const StringMap &map);

    // called by emulator while locked
    void resizeFd(Size size);
    bool setAttribute(const std::string &key, const std::string &value);
    bool removeAttribute(const std::string &key);
    const std::string& getAttribute(const std::string &key, bool *found = nullptr) const;
    const char* getAnswerback() const;
    const StringMap& resetEnviron() const;

    // called by emulator while locked
    std::string termCommand(const Codestring &body);
    void termData(const Codestring &body);
};
