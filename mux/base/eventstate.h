// Copyright © 2018 TermySequence LLC
//
// SPDX-License-Identifier: GPL-2.0-only

#pragma once

#include "cell.h"
#include "cursor.h"
#include "attributemap.h"
#include "region.h"

#include <set>
#include <map>
#include <vector>

class TermEmulator;
class TermInstance;
class TermReader;
class TermProxy;
class BaseWatch;
class TermWatch;
class TermProxyWatch;

#define MAX_QUEUED_REGIONS 512

struct TermEventFlags
{
    bool flagsChanged;
    bool bufferChanged[2][2];
    bool bufferSwitched;
    bool sizeChanged;
    bool cursorChanged;
    bool rowsChanged;
    bool regionsChanged;
    bool mouseMoved;
    uint32_t bellCount;

    void setEventFlags();
};

struct TermEventBase: TermEventFlags
{
    std::set<index_t> changedRows[2];
    std::set<bufreg_t> changedRegions;

    inline TermEventBase(): TermEventFlags{} {}
    void clearEventState();
};

struct ProxyAccumulatedState: TermEventBase
{
    // accumulated state
    std::map<index_t,std::string> proxyRows[2];
    std::map<bufreg_t,std::string> proxyRegions;

    AttributeMap changedFiles;
    AttributeMap files;
    bool filesChanged;

    Size size;
    index_t bufSize[2];

    std::string flagsStr;
    std::string bufferCapacityStr[2];
    std::string bufferLengthStr[2];
    std::string bufferSwitchStr;
    std::string sizeStr;
    std::string cursorStr;
    std::string bellStr;
    std::string mouseStr;

    ProxyAccumulatedState();
};

struct ProxyEventState: TermEventBase
{
    TermProxy *m_proxy;

    // locked
    void pushContent();

    ProxyEventState(TermProxy *proxy);
};

struct TermEventState: TermEventBase
{
    TermEmulator *m_emulator;
    TermInstance *m_term;

    // locked
    void pushContent();
    void pullScreen();
    void pullEverything();

    TermEventState(TermInstance *term);
};

struct TermEventTransfer: TermEventFlags
{
    int watchType;

    uint32_t buf[16];
    index_t bufSize[2];
    uint32_t bufCaporder[2];
    uint32_t activeBuffer;
    uint64_t flags;
    Size size;
    Rect margins;
    CursorBase cursor;
    Point mousePos;

    std::vector<std::pair<index_t,CellRow>> outRows[2];
    std::vector<Region> outRegions;

    AttributeMap attributes;
    AttributeMap files;

    std::vector<std::pair<uint32_t,std::string>> proxyData;

    void transferBaseState(BaseWatch *watch);
    void transferTermState(TermWatch *watch);
    void transferProxyState(TermProxyWatch *watch);

    void writeProxyResponses(TermReader *reader);
    void writeTermResponses(TermReader *reader);
    void writeBaseResponses(TermReader *reader);
    void writeClosing(TermReader *reader, unsigned reason);
};

inline void
TermEventBase::clearEventState()
{
    flagsChanged = false;
    bufferChanged[0][0] = false;
    bufferChanged[0][1] = false;
    bufferChanged[1][0] = false;
    bufferChanged[1][1] = false;
    bufferSwitched = false;
    sizeChanged = false;
    cursorChanged = false;
    rowsChanged = false;
    regionsChanged = false;
    mouseMoved = false;
    bellCount = 0;

    changedRows[0].clear();
    changedRows[1].clear();
    changedRegions.clear();
}
