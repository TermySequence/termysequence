// Copyright © 2018 TermySequence LLC
//
// SPDX-License-Identifier: GPL-2.0-only

#include "common.h"
#include "app/logging.h"
#include "scroll.h"
#include "scrollport.h"
#include "buffers.h"

TermScroll::TermScroll(TermScrollport *scrollport, QWidget *parent) :
    QScrollBar(Qt::Vertical, parent),
    m_scrollport(scrollport),
    m_buffers(scrollport->buffers())
{
    connect(m_buffers, SIGNAL(contentChanged()), SLOT(update()));
    connect(m_buffers, SIGNAL(bufferChanged()), SLOT(updateScroll()));
    connect(m_scrollport, SIGNAL(scrollChanged()), SLOT(updateScroll()));

    connect(m_scrollport, SIGNAL(scrollRequest(int)), SLOT(handleScrollRequest(int)));
    connect(m_scrollport, SIGNAL(inputReceived()), SLOT(handleInputReceived()));
    connect(this, SIGNAL(actionTriggered(int)), SLOT(handleActionTriggered()));

    updateScroll();
}

void
TermScroll::updateScroll()
{
    int pageSize = m_scrollport->height();
    // NOTE this is where size_t collides with int for scrollback buffer sizing
    int max = (int)m_buffers->size() - pageSize;

    if (max < 0)
        max = 0;

    setPageStep(pageSize);
    setMaximum(max);
    setValue(m_scrollport->offset());

    // qCDebug(lcLayout, "scroll: (%d, %d, %d)", maximum(), pageStep(), value());
}

int
TermScroll::scrollTo(int pos)
{
    if (pos < 0)
        pos = 0;

    if (pos >= maximum()) {
        m_scrollport->setRow(INVALID_INDEX, true);
        pos = maximum();
    }
    else {
        m_scrollport->setRow(m_buffers->origin() + pos, true);
    }

    return pos;
}

void
TermScroll::handleScrollRequest(int type)
{
    int pos = value();

    switch (type)
    {
    case SCROLLREQ_TOP:
        pos = 0;
        break;
    case SCROLLREQ_BOTTOM:
        pos = maximum();
        break;
    case SCROLLREQ_PAGEUP:
        pos -= m_scrollport->height() / 2;
        break;
    case SCROLLREQ_PAGEDOWN:
        pos += m_scrollport->height() / 2;
        break;
    case SCROLLREQ_LINEUP:
        --pos;
        break;
    case SCROLLREQ_LINEDOWN:
        ++pos;
        break;
    }

    setValue(scrollTo(pos));
}

void
TermScroll::handleInputReceived()
{
    // if (m_scrollport->locked() && m_scrollport->primary()) {
    // if (m_scrollport->locked() && m_scrollport->focused()) {
    if (m_scrollport->locked()) {
        handleScrollRequest(SCROLLREQ_BOTTOM);
    }
}

void
TermScroll::handleActionTriggered()
{
    scrollTo(sliderPosition());
}

void
TermScroll::forwardWheelEvent(QWheelEvent *event)
{
    QScrollBar::wheelEvent(event);
}
