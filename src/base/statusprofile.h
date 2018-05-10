// Copyright © 2018 TermySequence LLC
//
// SPDX-License-Identifier: GPL-2.0-only

#pragma once

#include "statuslabel.h"

class TermManager;
class TermInstance;
class ProfileSettings;

class StatusProfile final: public StatusLabel
{
    Q_OBJECT

private:
    TermInstance *m_term = nullptr;
    TermManager *m_manager;

    QString m_profileName;

    QMetaObject::Connection m_mocProfile;

private slots:
    void handleTermActivated(TermInstance *term);

    void handleProfileChanged(const ProfileSettings *profile);

    void handleEdit();

protected:
    void contextMenuEvent(QContextMenuEvent *event);

public:
    StatusProfile(TermManager *manager);
};
