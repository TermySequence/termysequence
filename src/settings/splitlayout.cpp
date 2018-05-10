// Copyright © 2018 TermySequence LLC
//
// SPDX-License-Identifier: GPL-2.0-only

#include "common.h"
#include "splitlayout.h"
#include "lib/exception.h"

#define SPLIT_VERSION   1
#define MAX_SPLIT_DEPTH 3

//
// Parser
//
SplitLayoutReader::SplitLayoutReader() : m_ok(false)
{
}

void
SplitLayoutReader::parseStateRecords()
{
    unsigned count = m_unm.parseNumber();
    m_viewportCounts.append(count);

    while (count--) {
        ScrollportState state;
        state.id = m_unm.parseUuid();
        state.offset = m_unm.parseNumber64();
        state.modtimeRow = m_unm.parseNumber64();
        state.modtime = m_unm.parseNumber();
        state.activeJob = m_unm.parseNumber();
        m_viewports.append(state);
    }
}

bool
SplitLayoutReader::parseItem(unsigned d)
{
    if (d++ > MAX_SPLIT_DEPTH)
        return false;

    unsigned type = m_unm.parseNumber();
    m_types.append(type);

    QList<int> sizes;
    TermState termrec;

    switch (type) {
    case SPLIT_EMPTY:
        return true;
    case SPLIT_LOCAL:
    case SPLIT_REMOTE:
        termrec.termId = m_unm.parseUuid();
        termrec.serverId = m_unm.parseUuid();
        termrec.profileName = m_unm.parsePaddedString();
        termrec.islocal = (type == SPLIT_LOCAL);
        m_terms.append(termrec);
        parseStateRecords();
        return true;
    case SPLIT_HRESIZE2:
    case SPLIT_VRESIZE2:
    case SPLIT_HFIXED2:
    case SPLIT_VFIXED2:
        sizes.append(m_unm.parseNumber());
        sizes.append(m_unm.parseNumber());
        m_sizes.append(sizes);
        return parseItem(d) && parseItem(d);
    case SPLIT_HRESIZE3:
    case SPLIT_VRESIZE3:
    case SPLIT_HFIXED3:
    case SPLIT_VFIXED3:
        sizes.append(m_unm.parseNumber());
        sizes.append(m_unm.parseNumber());
        sizes.append(m_unm.parseNumber());
        m_sizes.append(sizes);
        return parseItem(d) && parseItem(d) && parseItem(d);
    case SPLIT_HRESIZE4:
    case SPLIT_VRESIZE4:
    case SPLIT_HFIXED4:
    case SPLIT_VFIXED4:
        sizes.append(m_unm.parseNumber());
        sizes.append(m_unm.parseNumber());
        sizes.append(m_unm.parseNumber());
        sizes.append(m_unm.parseNumber());
        m_sizes.append(sizes);
        return parseItem(d) && parseItem(d) && parseItem(d) && parseItem(d);
    default:
        return false;
    }
}

void
SplitLayoutReader::parse(const QByteArray &bytes)
{
    m_ok = false;
    m_bytes = bytes;
    m_unm.begin(m_bytes.data(), m_bytes.size());

    try {
        unsigned version = m_unm.parseOptionalNumber();
        unsigned length = m_unm.parseOptionalNumber();

        if (version != SPLIT_VERSION || length != m_unm.remainingLength())
            return;

        m_focusPos = m_unm.parseNumber();

        m_ok = parseItem(0) && m_unm.remainingLength() == 0;
    }
    catch (const Tsq::ProtocolException &e) {
        // bad parse
        m_ok = false;
    }
}

unsigned
SplitLayoutReader::nextType()
{
    if (m_ok) {
        unsigned result = m_types.front();
        m_types.pop_front();
        return result;
    }

    return SPLIT_EMPTY;
}

//
// Resize parameters
//
TermState
SplitLayoutReader::nextTermRecord()
{
    TermState result = m_terms.front();
    m_terms.pop_front();
    return result;
}

unsigned
SplitLayoutReader::nextViewportCount()
{
    unsigned result = m_viewportCounts.front();
    m_viewportCounts.pop_front();
    return result;
}

ScrollportState
SplitLayoutReader::nextViewportRecord()
{
    ScrollportState result = m_viewports.front();
    m_viewports.pop_front();
    return result;
}

//
// Split parameters
//
QList<int> SplitLayoutReader::nextSizes()
{
    QList<int> result = m_sizes.front();
    m_sizes.pop_front();
    return result;
}

//
// Serializer
//
SplitLayoutWriter::SplitLayoutWriter()
{
    m_marshaler.begin(SPLIT_VERSION);
}

QByteArray
SplitLayoutWriter::result() const
{
    return QByteArray(m_marshaler.resultPtr(), m_marshaler.length());
}

void
SplitLayoutWriter::setFocusPos(unsigned focusPos)
{
    m_marshaler.addNumber(focusPos);
}

void
SplitLayoutWriter::addEmptyPane()
{
    m_marshaler.addNumber(SPLIT_EMPTY);
}

void
SplitLayoutWriter::addPane(const TermState &rec, const QVector<ScrollportState> &state)
{
    m_marshaler.addNumber(rec.islocal ? SPLIT_LOCAL : SPLIT_REMOTE);
    m_marshaler.addUuid(rec.termId);
    m_marshaler.addUuid(rec.serverId);
    m_marshaler.addPaddedString(rec.profileName.toStdString());
    m_marshaler.addNumber(state.size());

    for (auto &i: state) {
        m_marshaler.addUuid(i.id);
        m_marshaler.addNumber64(i.offset);
        m_marshaler.addNumber64(i.modtimeRow);
        m_marshaler.addNumber(i.modtime);
        m_marshaler.addNumber(i.activeJob);
    }
}

void
SplitLayoutWriter::addResize(bool horizontal, const QList<int> &sizes)
{
    switch (sizes.size()) {
    case 0:
    case 1:
        addEmptyPane();
        return;
    case 2:
        m_marshaler.addNumber(horizontal ? SPLIT_HRESIZE2 : SPLIT_VRESIZE2);
        break;
    case 3:
        m_marshaler.addNumber(horizontal ? SPLIT_HRESIZE3 : SPLIT_VRESIZE3);
        break;
    default:
        m_marshaler.addNumber(horizontal ? SPLIT_HRESIZE4 : SPLIT_VRESIZE4);
        break;
    }

    for (int i = 0; i < sizes.size() && i < MAX_SPLIT_WIDGETS; ++i)
        m_marshaler.addNumber(sizes[i]);
}

void
SplitLayoutWriter::addFixed(bool horizontal, const QList<int> &sizes)
{
    switch (sizes.size()) {
    case 0:
    case 1:
        addEmptyPane();
        break;
    case 2:
        m_marshaler.addNumber(horizontal ? SPLIT_HFIXED2 : SPLIT_VFIXED2);
        break;
    case 3:
        m_marshaler.addNumber(horizontal ? SPLIT_HFIXED3 : SPLIT_VFIXED3);
        break;
    default:
        m_marshaler.addNumber(horizontal ? SPLIT_HFIXED4 : SPLIT_VFIXED4);
        break;
    }

    for (int i = 0; i < sizes.size() && i < MAX_SPLIT_WIDGETS; ++i)
        m_marshaler.addNumber(sizes[i]);
}
