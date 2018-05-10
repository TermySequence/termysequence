// Copyright © 2018 TermySequence LLC
//
// SPDX-License-Identifier: GPL-2.0-only

#include "common.h"
#include "app/attrbase.h"
#include "statusinfo.h"
#include "statusarrange.h"
#include "manager.h"
#include "term.h"
#include "scrollport.h"
#include "screen.h"
#include "buffers.h"

#define TR_TEXT1 TL("window-text", "Emulator size and scrollback position")
#define TR_TEXT2 TL("window-text", "Click to view terminal information")

StatusInfo::StatusInfo(TermManager *manager) :
    m_manager(manager)
{
    connect(manager, SIGNAL(termActivated(TermInstance*,TermScrollport*)),
            SLOT(handleTermActivated(TermInstance*,TermScrollport*)));
    connect(this, SIGNAL(clicked()), SLOT(handleClicked()));

    setToolTip(TR_TEXT1 + '\n' + TR_TEXT2);
    updateMessage();
}

void
StatusInfo::handleClicked()
{
    if (m_term)
        m_manager->actionViewTerminalInfo(m_term->idStr());
}

void
StatusInfo::updateMessage()
{
    QString msg = L("%1x%2").arg(m_localSize.width()).arg(m_localSize.height());

    if (m_localSize != m_remoteSize) {
        msg.append(L(" [%1x%2]").arg(m_remoteSize.width()).arg(m_remoteSize.height()));
    }

    msg.append(L(" %1/%2").arg(m_offset).arg(m_size));

    setText(msg);
}

void
StatusInfo::handleTermActivated(TermInstance *term, TermScrollport *scrollport)
{
    if (m_term) {
        disconnect(m_mocOffset);
        disconnect(m_mocBuffer);
        m_term->disconnect(this);
    }

    if ((m_term = term)) {
        connect(term, SIGNAL(sizeChanged(QSize)), SLOT(handleRemoteSizeChanged(QSize)));
        connect(term, SIGNAL(localSizeChanged(QSize)), SLOT(handleLocalSizeChanged(QSize)));
        m_mocBuffer = connect(term->buffers(), SIGNAL(bufferChanged()), SLOT(handleBufferChanged()));

        m_scrollport = scrollport;
        m_mocOffset = connect(scrollport, SIGNAL(offsetChanged(int)), SLOT(handleOffsetChanged(int)));

        m_localSize = scrollport->size();
        m_remoteSize = term->screen()->size();
        index_t origin = term->buffers()->origin();
        m_offset = scrollport->offset() + origin;
        m_size = term->buffers()->size() + origin;
    } else {
        m_localSize = QSize(0, 0);
        m_remoteSize = QSize(0, 0);
        m_offset = 0;
        m_size = 0;
    }

    updateMessage();
}

void
StatusInfo::handleRemoteSizeChanged(QSize size)
{
    m_remoteSize = size;
    updateMessage();
}

void
StatusInfo::handleLocalSizeChanged(QSize size)
{
    m_localSize = size;
    updateMessage();
}

void
StatusInfo::handleBufferChanged()
{
    m_size = m_term->buffers()->size() + m_term->buffers()->origin();
    updateMessage();
}

void
StatusInfo::handleOffsetChanged(int offset)
{
    m_offset = offset + m_term->buffers()->origin();
    updateMessage();
}
