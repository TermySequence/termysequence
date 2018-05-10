// Copyright © 2018 TermySequence LLC
//
// SPDX-License-Identifier: GPL-2.0-only

#include "common.h"
#include "termproxy.h"
#include "proxywatch.h"
#include "conn.h"
#include "writer.h"
#include "parsemap.h"
#include "lib/enums.h"
#include "lib/protocol.h"
#include "lib/wire.h"
#include "os/logging.h"
#include "config.h"

TermProxy::TermProxy(ConnInstance *parent, const char *body, uint32_t length, bool isTerm) :
    BaseProxy(parent, TermProxyReleased),
    m_isTerm(isTerm)
{
    Tsq::ProtocolUnmarshaler unm(body, length);

    m_id = unm.parseUuid();
    m_hopId = unm.parseUuid();
    m_nHops = unm.parseNumber() + 1;

    if (isTerm) {
        size.setWidth(unm.parseNumber());
        size.setHeight(unm.parseNumber());
    }

    parseStringMap(unm, m_attributes);

    if (isTerm) {
        // Set up state strings with default values
        flagsStr.append(8, '\0');
        bufferLengthStr[0].append(8, '\0');
        bufferLengthStr[1].append(8, '\0');
        bufferSwitchStr.append(4, '\0');
        cursorStr.append(16, '\0');
        mouseStr.append(8, '\0');

        Tsq::ProtocolMarshaler m;
        m.addNumberPair(size.width(), size.height());
        m.addNumberPair(0, 0);
        m.addNumberPair(size.width(), size.height());
        sizeStr = m.result().substr(8);

        m.begin(0);
        m.addNumber64(0);
        m.addNumber(TERM_INIT_CAPORDER << 8);
        bufferCapacityStr[0] = m.result().substr(8);

        m.begin(0);
        m.addNumber64(0);
        m.addNumber(TERM_INIT_CAPORDER << 8 | 1);
        bufferCapacityStr[1] = m.result().substr(8);
    }
}

void
TermProxy::wireTermFlagsChanged(const char *body, uint32_t length)
{
    StateLock slock(this, true);
    flagsStr.assign(body, length);
    flagsChanged = true;
}

void
TermProxy::wireTermBufferCapacity(const char *body, uint32_t length)
{
    if (length > 8) {
        int bufid = body[8] & 1;
        index_t len = le64toh(*reinterpret_cast<const uint64_t*>(body));

        StateLock slock(this, true);
        bufSize[bufid] = len;
        bufferCapacityStr[bufid].assign(body, length);
        bufferChanged[bufid][1] = true;

        auto &ref = proxyRows[bufid];
        while (!ref.empty() && ref.rbegin()->first >= len)
            ref.erase(std::prev(ref.end()));
        while (ref.size() > size.height())
            ref.erase(ref.begin());
    }
}

void
TermProxy::wireTermBufferLength(const char *body, uint32_t length)
{
    if (length > 8) {
        int bufid = body[8] & 1;
        index_t len = le64toh(*reinterpret_cast<const uint64_t*>(body));

        StateLock slock(this, true);
        bufSize[bufid] = len;
        bufferLengthStr[bufid].assign(body, length);
        bufferChanged[bufid][0] = true;

        auto &ref = proxyRows[bufid];
        while (!ref.empty() && ref.rbegin()->first >= len)
            ref.erase(std::prev(ref.end()));
        while (ref.size() > size.height())
            ref.erase(ref.begin());
    }
}

void
TermProxy::wireTermBufferSwitched(const char *body, uint32_t length)
{
    StateLock slock(this, true);
    bufferSwitchStr.assign(body, length);
    bufferSwitched = true;
}

void
TermProxy::wireTermSizeChanged(const char *body, uint32_t length)
{
    if (length > 7) {
        StateLock slock(this, true);
        size.setWidth(le32toh(*reinterpret_cast<const uint32_t*>(body)));
        size.setHeight(le32toh(*reinterpret_cast<const uint32_t*>(body + 4)));

        sizeStr.assign(body, length);
        sizeChanged = true;
    }
}

void
TermProxy::wireTermCursorMoved(const char *body, uint32_t length)
{
    StateLock slock(this, true);
    cursorStr.assign(body, length);
    cursorChanged = true;
}

void
TermProxy::wireTermBellRang(const char *body, uint32_t length)
{
    if (length > 7 && memcmp(body, "\0\0\0", 4) == 0) {
        StateLock slock(this, true);
        bellStr.assign(body, length);
        uint32_t count = le32toh(*reinterpret_cast<const uint32_t*>(body + 4));
        if (bellCount) {
            uint32_t tmp = htole32(bellCount + count);
            bellStr.replace(4, 4, reinterpret_cast<char*>(&tmp), 4);
        }
        bellCount += count;
    }
}

void
TermProxy::wireTermRowChanged(const char *body, uint32_t length)
{
    if (length > 7) {
        index_t row = le64toh(*reinterpret_cast<const uint64_t*>(body));
        int bufid = body[8] & 1;
        StateLock slock(this, true);
        changedRows[bufid].insert(row);
        proxyRows[bufid][row].assign(body, length);
        rowsChanged = true;
    }
}

void
TermProxy::wireTermRegionChanged(const char *body, uint32_t length)
{
    if (length >= 40) {
        regionid_t region = le32toh(*reinterpret_cast<const uint32_t*>(body));
        bufreg_t bufreg = MAKE_BUFREG(body[4], region);

        index_t endRow = le32toh(*reinterpret_cast<const uint32_t*>(body + 28));
        endRow <<= 32;
        endRow |= le32toh(*reinterpret_cast<const uint32_t*>(body + 24));

        StateLock slock(this, true);
        regionsChanged = true;

        changedRegions.insert(bufreg);
        proxyRegions[bufreg].assign(body, length);

        while (proxyRegions.size() > MAX_QUEUED_REGIONS)
            proxyRegions.erase(proxyRegions.begin());
    }
}

void
TermProxy::wireTermDirectoryUpdate(const char *body, uint32_t length)
{
    StateLock slock(this, true);
    files.clear();
    files.emplace(g_mtstr, std::string(body, length));
    changedFiles = files;
    filesChanged = true;
}

void
TermProxy::wireTermFileUpdate(const char *body, uint32_t length)
{
    if (length > 28) {
        unsigned len = strnlen(body + 28, length - 28);
        if (len) {
            std::string name(body + 28, len);

            StateLock slock(this, true);
            files[name].assign(body, length);
            changedFiles[name].assign(body, length);
            filesChanged = true;
        }
    }
}

void
TermProxy::wireTermFileRemoved(const char *body, uint32_t length)
{
    if (length > 8) {
        unsigned len = strnlen(body + 8, length - 8);
        if (len) {
            std::string name(body + 8, len);

            StateLock slock(this, true);
            files.erase(name);
            changedFiles[name].assign(body, 8);
            filesChanged = true;
        }
    }
}

void
TermProxy::wireTermEndOutput()
{
    {
        Lock lock(this);

        for (auto w: m_watches) {
            auto *watch = static_cast<TermProxyWatch*>(w);
            BaseWatch::ActivatorLock wlock(watch);

            // two locks held
            watch->state.flagsChanged |= flagsChanged;

            watch->state.bufferChanged[0][0] |= bufferChanged[0][0];
            watch->state.bufferChanged[0][1] |= bufferChanged[0][1];
            watch->state.bufferChanged[1][0] |= bufferChanged[1][0];
            watch->state.bufferChanged[1][1] |= bufferChanged[1][1];
            watch->state.bufferSwitched |= bufferSwitched;

            watch->state.sizeChanged |= sizeChanged;
            watch->state.cursorChanged |= cursorChanged;
            watch->state.bellCount += bellCount;

            if (rowsChanged || regionsChanged) {
                watch->state.rowsChanged |= rowsChanged;
                watch->state.regionsChanged |= regionsChanged;
                watch->state.pushContent();
            }
            if (filesChanged) {
                watch->pushFileUpdates(changedFiles);
            }
        }
    }

    // Reset
    clearEventState();
    changedFiles.clear();
    filesChanged = false;
}

void
TermProxy::wireTermMouseMoved(const char *body, uint32_t length)
{
    {
        StateLock slock(this, true);
        mouseStr.assign(body, length);
    }

    Lock lock(this);

    for (auto w: m_watches) {
        auto *watch = static_cast<TermProxyWatch*>(w);
        BaseWatch::ActivatorLock wlock(watch);
        watch->state.mouseMoved = true;
    }
}

void
TermProxy::wireCommand(uint32_t command, uint32_t length, const char *body)
{
    switch (command) {
    case TSQ_BEGIN_OUTPUT:
        break;
    case TSQ_FLAGS_CHANGED:
        wireTermFlagsChanged(body, length);
        break;
    case TSQ_BUFFER_CAPACITY:
        wireTermBufferCapacity(body, length);
        break;
    case TSQ_BUFFER_LENGTH:
        wireTermBufferLength(body, length);
        break;
    case TSQ_BUFFER_SWITCHED:
        wireTermBufferSwitched(body, length);
        break;
    case TSQ_SIZE_CHANGED:
        wireTermSizeChanged(body, length);
        break;
    case TSQ_CURSOR_MOVED:
        wireTermCursorMoved(body, length);
        break;
    case TSQ_BELL_RANG:
        wireTermBellRang(body, length);
        break;
    case TSQ_ROW_CONTENT:
        wireTermRowChanged(body, length);
        break;
    case TSQ_REGION_UPDATE:
        wireTermRegionChanged(body, length);
        break;
    case TSQ_DIRECTORY_UPDATE:
        wireTermDirectoryUpdate(body, length);
        break;
    case TSQ_FILE_UPDATE:
        wireTermFileUpdate(body, length);
        break;
    case TSQ_FILE_REMOVED:
        wireTermFileRemoved(body, length);
        break;
    case TSQ_END_OUTPUT:
        wireTermEndOutput();
        break;
    case TSQ_MOUSE_MOVED:
        wireTermMouseMoved(body, length);
        break;
    case TSQ_GET_TERM_ATTRIBUTE:
        wireAttribute(body, length);
        break;
    default:
        LOGNOT("Term %p: unrecognized command %x\n", m_parent, command);
        break;
    }
}
