// Copyright © 2018 TermySequence LLC
//
// SPDX-License-Identifier: GPL-2.0-only

#include "common.h"
#include "termwatch.h"
#include "term.h"
#include "emulator.h"
#include "writer.h"
#include "filemon.h"
#include "listener.h"
#include "lib/protocol.h"
#include "lib/wire.h"

TermWatch::TermWatch(TermInstance *parent, TermReader *reader) :
    ConnWatch(parent, reader, WatchTerm),
    m_emulator(parent->emulator()),
    state(parent)
{
}

void
TermWatch::pullFiles()
{
    TermFilemon::StateLock slock(term()->filemon(), false);

    files = term()->filemon()->files();
}

void
TermWatch::pullEverything()
{
    ActivatorLock wlock(this);

    // two locks held
    state.pullEverything();
    // two locks held
    pullFiles();
}

void
TermWatch::pushAnnounce()
{
    Tsq::ProtocolMarshaler m(TSQ_ANNOUNCE_TERM);
    m.addUuidPair(term()->id(), g_listener->id());
    m.addNumber(0);

    {
        // two locks held
        TermInstance::StateLock slock(term(), false);

        Size size = m_emulator->size();
        m.addNumberPair(size.width(), size.height());
        m.addBytes(term()->lockedGetAttributes());

        state.pullScreen();
    }

    // two locks held
    m_writer->submitResponse(std::move(m.result()));

    // two locks held
    pullFiles();
}

void
TermWatch::pushUserRegion(bufreg_t region)
{
    Lock wlock(this);

    if (!closing) {
        while (state.changedRegions.size() >= MAX_QUEUED_REGIONS)
            state.changedRegions.erase(state.changedRegions.begin());

        state.changedRegions.emplace(region);
        state.regionsChanged = true;
        m_writer->activate(this);
    }
}

void
TermWatch::pushDirectoryUpdate(const std::string &msg)
{
    Lock wlock(this);

    if (active) {
        files.clear();
        files.emplace(g_mtstr, msg);
        m_writer->activate(this);
    }
}

void
TermWatch::pushFileUpdate(const std::string &name, const std::string &msg)
{
    Lock wlock(this);

    if (active) {
        files[name] = msg;
        m_writer->activate(this);
    }
}

void
TermWatch::pushFileUpdates(const StringMap &map)
{
    Lock wlock(this);

    if (active) {
        for (const auto &i: map)
            files[i.first] = i.second;

        m_writer->activate(this);
    }
}
