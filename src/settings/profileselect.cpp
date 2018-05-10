// Copyright © 2018 TermySequence LLC
//
// SPDX-License-Identifier: GPL-2.0-only

#include "common.h"
#include "app/attrbase.h"
#include "profileselect.h"
#include "profile.h"
#include "settings.h"
#include "profilewindow.h"

#include <QPushButton>
#include <QComboBox>
#include <QHBoxLayout>

#define TR_BUTTON1 TL("input-button", "Manage Profiles") + A("...")
#define TR_SETTING1 TL("settings-enum", "Use default profile")
#define TR_SETTING2 TL("settings-enum", "Use server's default profile")

ProfileSelect::ProfileSelect(const SettingDef *def, SettingsBase *settings,
                             bool serverDefault) :
    SettingWidget(def, settings)
{
    m_deftext = serverDefault ? TR_SETTING2 : TR_SETTING1;

    m_combo = new QComboBox();
    m_combo->installEventFilter(this);
    populateValues();

    QPushButton *button = new QPushButton(TR_BUTTON1);

    QHBoxLayout *layout = new QHBoxLayout;
    layout->setContentsMargins(g_mtmargins);
    layout->addWidget(m_combo, 1);
    layout->addWidget(button);
    setLayout(layout);

    connect(g_settings, SIGNAL(profilesChanged()), SLOT(handleProfilesChanged()));
    connect(button, SIGNAL(clicked()), SLOT(handleClicked()));
    m_mocCombo = connect(m_combo, SIGNAL(currentIndexChanged(int)),
                         SLOT(handleIndexChanged(int)));
}

void
ProfileSelect::populateValues()
{
    m_combo->clear();
    m_combo->addItem(m_deftext);
    m_combo->insertSeparator(1);

    for (auto i: g_settings->allProfiles()) {
        m_combo->addItem(i.second, i.first, i.first);
    }

    handleSettingChanged(m_value);
}

void
ProfileSelect::handleIndexChanged(int index)
{
    disconnect(m_mocCombo);
    setProperty(m_combo->itemData(index));
    m_mocCombo = connect(m_combo, SIGNAL(currentIndexChanged(int)),
                         SLOT(handleIndexChanged(int)));
}

void
ProfileSelect::handleProfilesChanged()
{
    disconnect(m_mocCombo);
    populateValues();
    m_mocCombo = connect(m_combo, SIGNAL(currentIndexChanged(int)),
                         SLOT(handleIndexChanged(int)));
}

void
ProfileSelect::handleSettingChanged(const QVariant &value)
{
    int idx = m_combo->findData(value.toString());
    m_combo->setCurrentIndex(idx != -1 ? idx : 0);
}

void
ProfileSelect::handleClicked()
{
    if (g_profilewin == nullptr)
        g_profilewin = new ProfilesWindow();

    g_profilewin->bringUp();
}


ProfileSelectFactory::ProfileSelectFactory(bool serverDefault) :
    m_serverDefault(serverDefault)
{
}

QWidget *
ProfileSelectFactory::createWidget(const SettingDef *def, SettingsBase *settings) const
{
    return new ProfileSelect(def, settings, m_serverDefault);
}
