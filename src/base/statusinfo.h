// Copyright © 2018 TermySequence LLC
//
// SPDX-License-Identifier: GPL-2.0-only

#pragma once

#include "statuslabel.h"
#include "lib/types.h"

class TermManager;
class TermInstance;
class TermScrollport;

class StatusInfo final: public StatusLabel
{
    Q_OBJECT

private:
    TermInstance *m_term = nullptr;
    TermScrollport *m_scrollport;
    TermManager *m_manager;

    QSize m_localSize{0,0}, m_remoteSize{0,0};
    index_t m_offset = 0, m_size = 0;

    QMetaObject::Connection m_mocBuffer, m_mocOffset;

    void updateMessage();

private slots:
    void handleTermActivated(TermInstance *term, TermScrollport *scrollport);

    void handleRemoteSizeChanged(QSize size);
    void handleLocalSizeChanged(QSize size);

    void handleBufferChanged();
    void handleOffsetChanged(int);

    void handleClicked();

public:
    StatusInfo(TermManager *manager);
};
