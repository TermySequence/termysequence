// Copyright © 2018 TermySequence LLC
//
// SPDX-License-Identifier: GPL-2.0-only

#include "common.h"
#include "app/config.h"
#include "app/logging.h"
#include "fetchtimer.h"
#include "listener.h"
#include "term.h"
#include "buffers.h"
#include "screen.h"
#include "server.h"
#include "conn.h"
#include "settings/global.h"
#include "os/time.h"

#include <QTimerEvent>

static inline int idleInterval(int fetchSpeed) {
    return (fetchSpeed > FETCH_IDLE_MIN) ? fetchSpeed : FETCH_IDLE_MIN;
}
static inline int idleThreshold(int fetchSpeed) {
    return idleInterval(fetchSpeed) - FETCH_IDLE_MIN;
}

TermFetchTimer::TermFetchTimer(TermListener *parent) :
    QObject(parent)
{
    m_iter = m_active.end();

    connect(parent, SIGNAL(termAdded(TermInstance*)), SLOT(handleTermAdded(TermInstance*)));
    connect(parent, SIGNAL(termRemoved(TermInstance*)), SLOT(handleTermRemoved(TermInstance*)));

    connect(g_global, SIGNAL(fetchSpeedChanged(int)), SLOT(handleSpeedChanged(int)));
    int speed = g_global->fetchSpeed();
    if ((m_enabled = (speed >= 0)))
        m_atimerInterval = speed;

    m_idleThreshold = idleThreshold(speed);
    m_stimerInterval = idleInterval(speed);
}

void
TermFetchTimer::handleTermAdded(TermInstance *term)
{
    TermBuffers *buffers = term->buffers();
    connect(buffers, SIGNAL(fetchFetchRow()), SLOT(handleRowFetched()));
    connect(buffers, SIGNAL(bufferChanged()), SLOT(handleBufferChanged()));
    connect(buffers, SIGNAL(bufferReset()), SLOT(handleBufferReset()));
    connect(term, SIGNAL(throttleChanged(bool)), SLOT(handleThrottleChanged(bool)));
    connect(term, SIGNAL(ratelimitChanged(bool)), SLOT(handleRatelimitChanged(bool)));
}

void
TermFetchTimer::disableTerm(TermBuffers *buffers)
{
    // qCDebug(lcTerm) << this << "disabling term" << buffers;

    m_staged.remove(buffers);
    if (m_staged.isEmpty() && m_stimerId) {
        killTimer(m_stimerId);
        m_stimerId = 0;
    }

    m_active.remove(buffers);
    m_iter = m_active.begin();
    if (m_active.isEmpty() && m_atimerId) {
        killTimer(m_atimerId);
        m_atimerId = 0;
    }
}

void
TermFetchTimer::stageTerm(TermBuffers *buffers)
{
    // qCDebug(lcTerm) << this << "staging term" << buffers;

    if (!buffers->term()->throttled() &&
        !(buffers->term()->flags() & Tsq::RateLimited))
    {
        bool wasEmpty = m_staged.isEmpty();

        m_staged.insert(buffers);

        if (wasEmpty && m_enabled && m_stimerId == 0) {
            m_stimerId = startTimer(m_stimerInterval);
        }
    }
}

void
TermFetchTimer::activateTerm(TermBuffers *buffers)
{
    // qCDebug(lcTerm) << this << "activating term" << buffers;

    if (!buffers->term()->throttled() &&
        !(buffers->term()->flags() & Tsq::RateLimited))
    {
        bool wasEmpty = m_active.isEmpty();

        m_iter = m_active.insert(buffers);

        if (wasEmpty && m_enabled && m_atimerId == 0) {
            m_atimerId = startTimer(m_atimerInterval);
        }
    }
}

void
TermFetchTimer::handleTermRemoved(TermInstance *term)
{
    TermBuffers *buffers = term->buffers();
    buffers->disconnect(this);
    term->disconnect(this);

    disableTerm(buffers);
}

void
TermFetchTimer::handleRowFetched()
{
    TermBuffers *buffers = static_cast<TermBuffers*>(sender());
    buffers->setFetchNext(INVALID_INDEX);
    activateTerm(buffers);
}

void
TermFetchTimer::handleBufferChanged()
{
    TermBuffers *buffers = static_cast<TermBuffers*>(sender());
    stageTerm(buffers);
}

void
TermFetchTimer::handleBufferReset()
{
    TermBuffers *buffers = static_cast<TermBuffers*>(sender());

    buffers->setFetchNext(INVALID_INDEX);
    buffers->setFetchPos(0l);
}

void
TermFetchTimer::handleThrottleChanged(bool throttled)
{
    TermInstance *term = static_cast<TermInstance*>(sender());

    if (throttled || term->flags() & Tsq::RateLimited)
        disableTerm(term->buffers());
    else
        stageTerm(term->buffers());
}

void
TermFetchTimer::handleRatelimitChanged(bool ratelimited)
{
    TermInstance *term = static_cast<TermInstance*>(sender());

    if (ratelimited || term->throttled())
        disableTerm(term->buffers());
    else
        stageTerm(term->buffers());
}

bool
TermFetchTimer::activeTimerHelper(int64_t now)
{
    TermBuffers *buffers = *m_iter;
    TermInstance *term = buffers->term();
    ServerConnection *conn = term->server()->conn();
    bool done = false, sent = false;

    if (!conn) {
        done = true;
    } else if (now - conn->timestamp() < m_idleThreshold) {
        stageTerm(buffers);
        done = true;
    } else {
        index_t fetchPos = buffers->fetchPos();
        index_t origin = buffers->origin();
        size_t size = buffers->size0();
        size_t lower = term->screen()->height();

        size -= (size > lower) ? lower : size;

        if (fetchPos < origin)
            fetchPos = origin;

        lower = fetchPos - origin;

        for (int i = 0; i < FETCH_PREFETCH; ++i, ++lower)
        {
            if (lower >= size) {
                done = true;
                break;
            }

            if ((buffers->row(lower).flags & Tsqt::Downloaded) == 0)
            {
                size_t upper = lower + FETCH_PREFETCH;
                if (upper > size)
                    upper = size;

                buffers->setFetchNext(upper = origin + upper - 1);
                g_listener->pushTermFetch(term, origin + lower, upper, 0);
                done = sent = true;
                break;
            }
        }

        buffers->setFetchPos(origin + lower);
    }

    if (done)
        m_iter = m_active.erase(m_iter);
    else
        ++m_iter;

    return sent;
}

void
TermFetchTimer::timerEvent(QTimerEvent *event)
{
    int64_t now = osMonotime();

    if (event->timerId() == m_atimerId) {
        // qCDebug(lcTerm) << this << "atimer:" << m_active.size() << "terms active";
        for (int i = 0; i < FETCH_BATCH_SIZE; ++i) {
            if (m_active.isEmpty()) {
                if (m_atimerId) {
                    killTimer(m_atimerId);
                    m_atimerId = 0;
                }
                break;
            }
            if (m_iter == m_active.end()) {
                m_iter = m_active.begin();
            }
            if (activeTimerHelper(now)) {
                break;
            }
        }
    } else {
        // qCDebug(lcTerm) << this << "stimer:" << m_staged.size() << "terms staged";
        for (auto i = m_staged.begin(); i != m_staged.end(); )
        {
            TermInstance *term = (*i)->term();
            ServerConnection *conn = term->server()->conn();

            if (!conn) {
                i = m_staged.erase(i);
            } else if (now - conn->timestamp() > m_idleThreshold) {
                activateTerm(*i);
                i = m_staged.erase(i);
            } else {
                ++i;
            }
        }

        if (m_staged.isEmpty() && m_stimerId) {
            killTimer(m_stimerId);
            m_stimerId = 0;
        }
    }
}

void
TermFetchTimer::handleSpeedChanged(int speed)
{
    if (m_stimerId) {
        killTimer(m_stimerId);
        m_stimerId = 0;
    }
    if (m_atimerId) {
        killTimer(m_atimerId);
        m_atimerId = 0;
    }

    if ((m_enabled = (speed >= 0))) {
        m_idleThreshold = idleThreshold(speed);

        m_atimerId = startTimer(m_atimerInterval = speed);
        m_stimerId = startTimer(m_stimerInterval = idleInterval(speed));
    }
}
