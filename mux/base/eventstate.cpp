// Copyright © 2018 TermySequence LLC
//
// SPDX-License-Identifier: GPL-2.0-only

#include "common.h"
#include "eventstate.h"
#include "emulator.h"
#include "buffer.h"
#include "term.h"
#include "termwatch.h"
#include "termproxy.h"
#include "proxywatch.h"
#include "reader.h"
#include "lib/wire.h"
#include "lib/machine.h"
#include "lib/protocol.h"

inline void
TermEventFlags::setEventFlags()
{
    flagsChanged = true;
    bufferChanged[0][0] = true;
    bufferChanged[0][1] = true;
    bufferChanged[1][0] = true;
    bufferChanged[1][1] = true;
    bufferSwitched = true;
    sizeChanged = true;
    cursorChanged = true;
    rowsChanged = true;
    regionsChanged = true;
    mouseMoved = true;
}

ProxyAccumulatedState::ProxyAccumulatedState() :
    filesChanged(false),
    bufSize{0, 0}
{
}

ProxyEventState::ProxyEventState(TermProxy *proxy) :
    m_proxy(proxy)
{
    setEventFlags();
}

static inline void
combineRows(std::set<index_t> &dst, const std::set<index_t> &src,
            index_t size, unsigned screenHeight)
{
    dst.insert(src.begin(), src.end());

    // Note: possible bound multiplier would go here
    size -= screenHeight;

    for (auto i = dst.begin(); i != dst.end() && *i < size; i = dst.begin())
        dst.erase(i);
}

void
ProxyEventState::pushContent()
{
    // Note: we're not responsible for setting rowsChanged/regionsChanged

    // Buffer 1 rows
    combineRows(changedRows[1], m_proxy->changedRows[1],
                m_proxy->bufSize[1], m_proxy->size.height());

    // Buffer 0 rows
    combineRows(changedRows[0], m_proxy->changedRows[0],
                m_proxy->bufSize[0], m_proxy->size.height());

    // Regions
    changedRegions.insert(m_proxy->changedRegions.begin(), m_proxy->changedRegions.end());
    while (changedRegions.size() > MAX_QUEUED_REGIONS)
        changedRegions.erase(changedRegions.begin());
}

TermEventState::TermEventState(TermInstance *term) :
    m_emulator(term->emulator()),
    m_term(term)
{
    setEventFlags();
}

void
TermEventState::pushContent()
{
    // Note: we ARE responsible for setting rowsChanged/regionsChanged

    const auto *buf1 = m_emulator->buffer(1);
    const auto *buf0 = m_emulator->buffer(0);

    // Rows
    combineRows(changedRows[1], buf1->changedRows(),
                buf1->size(), buf1->screenHeight());
    combineRows(changedRows[0], buf0->changedRows(),
                buf0->size(), buf0->screenHeight());

    rowsChanged = !(changedRows[0].empty() && changedRows[1].empty());

    // Regions
    changedRegions.insert(buf1->changedRegions().begin(), buf1->changedRegions().end());
    changedRegions.insert(buf0->changedRegions().begin(), buf0->changedRegions().end());

    if (!changedRegions.empty()) {
        regionsChanged = true;

        while (changedRegions.size() > MAX_QUEUED_REGIONS)
            changedRegions.erase(changedRegions.begin());
    }
}

void
TermEventState::pullScreen()
{
    const TermBuffer *buffer = m_emulator->buffer(0);
    index_t end = buffer->size();
    index_t start = end - m_emulator->size().height();
    for (index_t i = start; i < end; ++i) {
        changedRows[0].emplace_hint(changedRows[0].end(), i);
    }
    buffer->pullRegions(start, end, changedRegions);

    buffer = m_emulator->buffer(1);
    end = buffer->size();
    start = end - m_emulator->size().height();
    for (index_t i = start; i < end; ++i) {
        changedRows[1].emplace_hint(changedRows[1].end(), i);
    }
    buffer->pullRegions(start, end, changedRegions);

    // Note: we're not responsible for setting rowsChanged/regionsChanged
}

void
TermEventState::pullEverything()
{
    setEventFlags();

    TermInstance::StateLock slock(m_term, false);
    pullScreen();
}

void
TermEventTransfer::transferBaseState(BaseWatch *watch)
{
    watchType = watch->type;

    // Id
    memcpy(buf + 2, watch->parentId().buf, 16);

    attributes.swap(watch->attributes);
}

void
TermEventTransfer::transferTermState(TermWatch *watch)
{
    auto &state = watch->state;

    TermEventFlags::operator=(state);

    {
        // Copy
        TermEmulator *emulator = state.m_emulator;
        TermInstance::StateLock slock(state.m_term, false);

        if (flagsChanged) {
            flags = emulator->flags();
        }
        if (bufferChanged[0][0]) {
            bufSize[0] = emulator->buffer(0)->size();
        }
        if (bufferChanged[0][1]) {
            bufCaporder[0] = emulator->buffer(0)->caporder() << 8;
        }
        if (bufferChanged[1][0]) {
            bufSize[1] = emulator->buffer(1)->size();
        }
        if (bufferChanged[1][1]) {
            bufCaporder[1] = emulator->buffer(1)->caporder() << 8 | 1;
        }
        if (bufferSwitched) {
            activeBuffer = emulator->altActive();
        }
        if (sizeChanged) {
            size = emulator->size();
            margins = emulator->margins();
        }
        if (cursorChanged) {
            cursor = emulator->cursor();
        }
        if (mouseMoved) {
            mousePos = emulator->mousePos();
        }

        // Content
        if (rowsChanged)
        {
            // Buffer 0
            const auto *buffer = emulator->buffer(0);
            index_t size = buffer->size();

            combineRows(state.changedRows[0], buffer->changedRows(),
                        size, buffer->screenHeight());

            for (index_t i: state.changedRows[0]) {
                if (i >= size)
                    break;
                outRows[0].emplace_back(i, buffer->constRow(i));
            }

            // Buffer 1
            buffer = emulator->buffer(1), size = buffer->size();
            for (index_t i: state.changedRows[1]) {
                if (i >= size)
                    break;
                outRows[1].emplace_back(i, buffer->constRow(i));
            }
        }

        // Regions
        if (regionsChanged)
        {
            const Region *region;
            for (bufreg_t i: state.changedRegions)
                if ((region = emulator->safeRegion(i)))
                    outRegions.push_back(*region);
        }
    }

    // Files
    files.swap(watch->files);

    // Reset
    state.clearEventState();
}

void
TermEventTransfer::transferProxyState(TermProxyWatch *watch)
{
    auto &state = watch->state;
    TermProxy *proxy = state.m_proxy;

    {
        // Copy
        TermProxy::StateLock slock(proxy, false);

        if (state.flagsChanged) {
            proxyData.emplace_back(TSQ_FLAGS_CHANGED, proxy->flagsStr);
        }

        if (state.bufferChanged[0][1]) {
            proxyData.emplace_back(TSQ_BUFFER_CAPACITY, proxy->bufferCapacityStr[0]);
        }
        else if (state.bufferChanged[0][0]) {
            proxyData.emplace_back(TSQ_BUFFER_LENGTH, proxy->bufferLengthStr[0]);
        }

        if (state.bufferChanged[1][1]) {
            proxyData.emplace_back(TSQ_BUFFER_CAPACITY, proxy->bufferCapacityStr[1]);
        }
        else if (state.bufferChanged[1][0]) {
            proxyData.emplace_back(TSQ_BUFFER_LENGTH, proxy->bufferLengthStr[1]);
        }

        if (state.bufferSwitched) {
            proxyData.emplace_back(TSQ_BUFFER_SWITCHED, proxy->bufferSwitchStr);
        }
        if (state.sizeChanged) {
            proxyData.emplace_back(TSQ_SIZE_CHANGED, proxy->sizeStr);
        }
        if (state.cursorChanged) {
            proxyData.emplace_back(TSQ_CURSOR_MOVED, proxy->cursorStr);
        }
        if (state.bellCount) {
            proxyData.emplace_back(TSQ_BELL_RANG, proxy->bellStr);
        }

        // Regions
        if (state.regionsChanged)
        {
            std::map<bufreg_t,std::string>::iterator j, k;

            j = proxy->proxyRegions.end();
            for (bufreg_t i: state.changedRegions)
                if ((k = proxy->proxyRegions.find(i)) != j)
                    proxyData.emplace_back(TSQ_REGION_UPDATE, k->second);
        }

        // Rows
        if (state.rowsChanged)
        {
            std::map<index_t,std::string>::iterator j, k;

            j = proxy->proxyRows[0].end();
            for (index_t i: state.changedRows[0])
                if ((k = proxy->proxyRows[0].find(i)) != j)
                    proxyData.emplace_back(TSQ_ROW_CONTENT, k->second);

            j = proxy->proxyRows[1].end();
            for (index_t i: state.changedRows[1])
                if ((k = proxy->proxyRows[1].find(i)) != j)
                    proxyData.emplace_back(TSQ_ROW_CONTENT, k->second);
        }

        // Mouse
        if ((mouseMoved = state.mouseMoved)) {
            proxyData.emplace_back(TSQ_MOUSE_MOVED, proxy->mouseStr);
        }
    }

    // Files
    files.swap(watch->files);

    // Reset
    state.clearEventState();
}

void
TermEventTransfer::writeProxyResponses(TermReader *reader)
{
    Tsq::ProtocolMachine *machine = reader->machine();
    size_t n = proxyData.size();

    if (mouseMoved)
        --n;
    if (n == 0 && files.empty())
        goto out;

    buf[0] = htole32(TSQ_BEGIN_OUTPUT);
    buf[1] = htole32(16);
    machine->connSend(reinterpret_cast<const char *>(buf), 24);

    for (size_t i = 0; i < n; ++i) {
        const auto &ref = proxyData[i];
        buf[0] = htole32(ref.first);
        buf[1] = htole32(16 + ref.second.size());
        machine->connSend(reinterpret_cast<const char *>(buf), 24);
        machine->connSend(ref.second.data(), ref.second.size());
    }

    if (!files.empty()) {
        auto i = files.find(g_mtstr);
        if (i != files.end()) {
            buf[0] = htole32(TSQ_DIRECTORY_UPDATE);
            buf[1] = htole32(16 + i->second.size());
            machine->connSend(reinterpret_cast<const char *>(buf), 24);
            machine->connSend(i->second.data(), i->second.size());
            files.erase(i);
        }
        for (const auto &i: files) {
            if (i.second.size() == 8) {
                buf[0] = htole32(TSQ_FILE_REMOVED);
                buf[1] = htole32(24 + i.first.size());
                memcpy(buf + 24, i.second.data(), 8);
                machine->connSend(reinterpret_cast<const char *>(buf), 32);
                machine->connSend(i.first.data(), i.first.size());
            } else {
                buf[0] = htole32(TSQ_FILE_UPDATE);
                buf[1] = htole32(16 + i.second.size());
                machine->connSend(reinterpret_cast<const char *>(buf), 24);
                machine->connSend(i.second.data(), i.second.size());
            }
        }

        files.clear();
    }

    buf[0] = htole32(TSQ_END_OUTPUT);
    buf[1] = htole32(16);
    machine->connSend(reinterpret_cast<const char *>(buf), 24);

out:
    if (mouseMoved) {
        const auto &ref = proxyData.back();
        buf[0] = htole32(ref.first);
        buf[1] = htole32(16 + ref.second.size());
        machine->connSend(reinterpret_cast<const char *>(buf), 24);
        machine->connSend(ref.second.data(), ref.second.size());
    }

    proxyData.clear();
}

void
TermEventTransfer::writeTermResponses(TermReader *reader)
{
    Tsq::ProtocolMachine *machine = reader->machine();
    bool haveOutput = flagsChanged || bufferChanged[0][0] || bufferChanged[0][1] ||
        bufferChanged[1][0] || bufferChanged[1][1] || bufferSwitched || sizeChanged ||
        cursorChanged || rowsChanged || regionsChanged || bellCount ||
        !files.empty();

    if (!haveOutput)
        goto out;

    buf[0] = htole32(TSQ_BEGIN_OUTPUT);
    buf[1] = htole32(16);
    machine->connSend(reinterpret_cast<const char *>(buf), 24);

    if (flagsChanged) {
        buf[0] = htole32(TSQ_FLAGS_CHANGED);
        buf[1] = htole32(24);
        buf[6] = htole32(flags);
        buf[7] = htole32(flags >> 32);
        machine->connSend(reinterpret_cast<const char *>(buf), 32);
    }

    if (bufferChanged[0][1]) {
        buf[0] = htole32(TSQ_BUFFER_CAPACITY);
        buf[1] = htole32(28);
        buf[6] = htole32(bufSize[0]);
        buf[7] = htole32(bufSize[0] >> 32);
        buf[8] = htole32(bufCaporder[0]);
        machine->connSend(reinterpret_cast<const char *>(buf), 36);
    }
    else if (bufferChanged[0][0]) {
        buf[0] = htole32(TSQ_BUFFER_LENGTH);
        buf[1] = htole32(28);
        buf[6] = htole32(bufSize[0]);
        buf[7] = htole32(bufSize[0] >> 32);
        buf[8] = htole32(0);
        machine->connSend(reinterpret_cast<const char *>(buf), 36);
    }

    if (bufferChanged[1][1]) {
        buf[0] = htole32(TSQ_BUFFER_CAPACITY);
        buf[1] = htole32(28);
        buf[6] = htole32(bufSize[1]);
        buf[7] = htole32(bufSize[1] >> 32);
        buf[8] = htole32(bufCaporder[1]);
        machine->connSend(reinterpret_cast<const char *>(buf), 36);
    }
    else if (bufferChanged[1][0]) {
        buf[0] = htole32(TSQ_BUFFER_LENGTH);
        buf[1] = htole32(28);
        buf[6] = htole32(bufSize[1]);
        buf[7] = htole32(bufSize[1] >> 32);
        buf[8] = htole32(1);
        machine->connSend(reinterpret_cast<const char *>(buf), 36);
    }

    if (bufferSwitched) {
        buf[0] = htole32(TSQ_BUFFER_SWITCHED);
        buf[1] = htole32(20);
        buf[6] = htole32(activeBuffer);
        machine->connSend(reinterpret_cast<const char *>(buf), 28);
    }

    if (sizeChanged) {
        buf[0] = htole32(TSQ_SIZE_CHANGED);
        buf[1] = htole32(40);
        buf[6] = htole32(size.width());
        buf[7] = htole32(size.height());
        buf[8] = htole32(margins.x());
        buf[9] = htole32(margins.y());
        buf[10] = htole32(margins.width());
        buf[11] = htole32(margins.height());
        machine->connSend(reinterpret_cast<const char *>(buf), 48);
    }

    if (cursorChanged) {
        buf[0] = htole32(TSQ_CURSOR_MOVED);
        buf[1] = htole32(32);
        buf[6] = htole32(cursor.x());
        buf[7] = htole32(cursor.y());
        buf[8] = htole32(cursor.pos());
        buf[9] = htole32(cursor.flags());
        machine->connSend(reinterpret_cast<const char *>(buf), 40);
    }

    if (bellCount) {
        buf[0] = htole32(TSQ_BELL_RANG);
        buf[1] = htole32(24);
        buf[6] = 0;
        buf[7] = htole32(bellCount);
        machine->connSend(reinterpret_cast<const char *>(buf), 32);
    }

    if (regionsChanged) {
        buf[0] = htole32(TSQ_REGION_UPDATE);
        for (const auto &region: outRegions)
        {
            std::string tmp;
            for (const auto &i: region.attributes) {
                tmp.append(i.first);
                tmp.push_back('\0');
                tmp.append(i.second);
                tmp.push_back('\0');
            }

            buf[1] = htole32(56 + tmp.size());
            buf[6] = htole32(region.id);
            buf[7] = htole32(region.wireType());
            buf[8] = htole32(region.flags);
            buf[9] = htole32(region.parent);
            buf[10] = htole32(region.startRow);
            buf[11] = htole32(region.startRow >> 32);
            buf[12] = htole32(region.endRow);
            buf[13] = htole32(region.endRow >> 32);
            buf[14] = htole32(region.startCol);
            buf[15] = htole32(region.endCol);
            machine->connSend(reinterpret_cast<const char *>(buf), 64);
            machine->connSend(tmp.data(), tmp.size());
        }

        outRegions.clear();
    }

    if (rowsChanged) {
        buf[0] = htole32(TSQ_ROW_CONTENT);
        for (auto &&i: outRows[0])
        {
            CellRow &row = i.second;
            uint32_t rangeSize = row.m_ranges.size() * 4;
            uint32_t strSize = row.m_str.size();

            #if __BYTE_ORDER__ == __ORDER_BIG_ENDIAN__
            for (unsigned z = 0; z < row.m_ranges.size(); ++z)
                row.m_ranges[z] = __builtin_bswap32(row.m_ranges[z]);
            #endif

            buf[1] = htole32(36 + rangeSize + strSize);
            buf[6] = htole32(i.first);
            buf[7] = htole32(i.first >> 32);
            buf[8] = htole32(row.flags);
            buf[9] = htole32(row.modtime);
            buf[10] = htole32(row.numRanges());
            machine->connSend(reinterpret_cast<const char *>(buf), 44);
            machine->connSend(reinterpret_cast<const char *>(row.m_ranges.data()), rangeSize);
            machine->connSend(row.m_str.data(), strSize);
        }

        for (auto &&i: outRows[1])
        {
            CellRow &row = i.second;
            uint32_t rangeSize = row.m_ranges.size() * 4;
            uint32_t strSize = row.m_str.size();

            #if __BYTE_ORDER__ == __ORDER_BIG_ENDIAN__
            for (unsigned z = 0; z < row.m_ranges.size(); ++z)
                row.m_ranges[z] = __builtin_bswap32(row.m_ranges[z]);
            #endif

            buf[1] = htole32(36 + rangeSize + strSize);
            buf[6] = htole32(i.first);
            buf[7] = htole32(i.first >> 32);
            buf[8] = htole32(row.flags|1);
            buf[9] = htole32(row.modtime);
            buf[10] = htole32(row.numRanges());
            machine->connSend(reinterpret_cast<const char *>(buf), 44);
            machine->connSend(reinterpret_cast<const char *>(row.m_ranges.data()), rangeSize);
            machine->connSend(row.m_str.data(), strSize);
        }

        outRows[0].clear();
        outRows[1].clear();
    }

    if (!files.empty()) {
        auto i = files.find(g_mtstr);
        if (i != files.end()) {
            machine->connSend(i->second.data(), i->second.size());
            files.erase(i);
        }
        for (const auto &i: files) {
            machine->connSend(i.second.data(), i.second.size());
        }

        files.clear();
    }

    buf[0] = htole32(TSQ_END_OUTPUT);
    buf[1] = htole32(16);
    machine->connSend(reinterpret_cast<const char *>(buf), 24);

out:
    if (mouseMoved) {
        buf[0] = htole32(TSQ_MOUSE_MOVED);
        buf[1] = htole32(24);
        buf[6] = htole32(mousePos.x());
        buf[7] = htole32(mousePos.y());
        machine->connSend(reinterpret_cast<const char *>(buf), 32);
    }
}

void
TermEventTransfer::writeBaseResponses(TermReader *reader)
{
    if (!attributes.empty()) {
        Tsq::ProtocolMachine *machine = reader->machine();

        switch (watchType) {
        case WatchTerm:
        case WatchTermProxy:
            buf[0] = htole32(TSQ_GET_TERM_ATTRIBUTE);
            break;
        case WatchConn:
        case WatchConnProxy:
            buf[0] = htole32(TSQ_GET_CONN_ATTRIBUTE);
            break;
        default:
            buf[0] = htole32(TSQ_GET_SERVER_ATTRIBUTE);
            break;
        }

        for (const auto &i: attributes) {
            buf[1] = htole32(16 + i.second.size());
            machine->connSend(reinterpret_cast<const char *>(buf), 24);
            machine->connSend(i.second.data(), i.second.size());
        }

        attributes.clear();
    }
}

void
TermEventTransfer::writeClosing(TermReader *reader, unsigned reason)
{
    switch (watchType) {
    case WatchTerm:
    case WatchTermProxy:
        buf[0] = htole32(TSQ_REMOVE_TERM);
        break;
    case WatchConn:
    case WatchConnProxy:
        buf[0] = htole32(TSQ_REMOVE_CONN);
        break;
    case WatchServer:
        buf[0] = htole32(TSQ_REMOVE_SERVER);
        break;
    default:
        return;
    }

    buf[1] = htole32(20);
    buf[6] = htole32(reason);
    reader->machine()->connSend(reinterpret_cast<const char *>(buf), 28);
}
