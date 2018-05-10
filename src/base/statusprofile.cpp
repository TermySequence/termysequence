// Copyright © 2018 TermySequence LLC
//
// SPDX-License-Identifier: GPL-2.0-only

#include "common.h"
#include "app/attrbase.h"
#include "statusprofile.h"
#include "manager.h"
#include "term.h"
#include "menubase.h"
#include "settings/settings.h"
#include "settings/profile.h"

#include <QContextMenuEvent>

#define TR_TEXT1 TL("window-text", "Click to edit profile %1")
#define TR_TEXT2 TL("window-text", "Right-click to switch profile")

StatusProfile::StatusProfile(TermManager *manager) :
    m_manager(manager)
{
    connect(manager, SIGNAL(termActivated(TermInstance*,TermScrollport*)),
            SLOT(handleTermActivated(TermInstance*)));

    setContextMenuPolicy(Qt::DefaultContextMenu);
    connect(this, SIGNAL(clicked()), SLOT(handleEdit()));
}

void
StatusProfile::handleEdit()
{
    m_manager->actionEditProfile(g_mtstr);
}

void
StatusProfile::handleTermActivated(TermInstance *term)
{
    if (m_term) {
        disconnect(m_mocProfile);
    }

    if ((m_term = term)) {
        m_mocProfile = connect(m_term, SIGNAL(profileChanged(const ProfileSettings*)),
                               SLOT(handleProfileChanged(const ProfileSettings*)));

        handleProfileChanged(m_term->profile());
    } else {
        handleProfileChanged(g_settings->defaultProfile());
    }
}

void
StatusProfile::handleProfileChanged(const ProfileSettings *profile)
{
    if (m_profileName != profile->name()) {
        setText(m_profileName = profile->name());
        setToolTip(TR_TEXT1.arg(m_profileName) + '\n' + TR_TEXT2);
    }
}

void
StatusProfile::contextMenuEvent(QContextMenuEvent *event)
{
    auto m = new DynamicMenu(m_term, "dmenu-profile", m_manager->parent(), this);
    connect(m, SIGNAL(aboutToHide()), m, SLOT(deleteLater()));
    m->enable(SwitchProfileMenu);
    m->popup(event->globalPos());
    event->accept();
}
