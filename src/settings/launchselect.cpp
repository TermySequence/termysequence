// Copyright © 2018 TermySequence LLC
//
// SPDX-License-Identifier: GPL-2.0-only

#include "common.h"
#include "app/attrbase.h"
#include "launchselect.h"
#include "launcher.h"
#include "settings.h"
#include "launchwindow.h"

#include <QPushButton>
#include <QComboBox>
#include <QHBoxLayout>

#define TR_BUTTON1 TL("input-button", "Manage Launchers") + A("...")
#define TR_SETTING1 TL("settings-enum", "Use default launcher")

LauncherSelect::LauncherSelect(const SettingDef *def, SettingsBase *settings) :
    SettingWidget(def, settings)
{
    m_combo = new QComboBox();
    m_combo->installEventFilter(this);
    populateValues();

    QPushButton *button = new QPushButton(TR_BUTTON1);

    QHBoxLayout *layout = new QHBoxLayout;
    layout->setContentsMargins(g_mtmargins);
    layout->addWidget(m_combo, 1);
    layout->addWidget(button);
    setLayout(layout);

    connect(g_settings, SIGNAL(launchersChanged()), SLOT(handleLaunchersChanged()));
    connect(button, SIGNAL(clicked()), SLOT(handleClicked()));
    m_mocCombo = connect(m_combo, SIGNAL(currentIndexChanged(int)),
                         SLOT(handleIndexChanged(int)));
}

void
LauncherSelect::populateValues()
{
    m_combo->clear();
    m_combo->addItem(TR_SETTING1);
    m_combo->insertSeparator(1);

    for (auto i: g_settings->allLaunchers()) {
        m_combo->addItem(i.second, i.first, i.first);
    }

    handleSettingChanged(m_value);
}

void
LauncherSelect::handleIndexChanged(int index)
{
    disconnect(m_mocCombo);
    setProperty(m_combo->itemData(index));
    m_mocCombo = connect(m_combo, SIGNAL(currentIndexChanged(int)),
                         SLOT(handleIndexChanged(int)));
}

void
LauncherSelect::handleLaunchersChanged()
{
    disconnect(m_mocCombo);
    populateValues();
    m_mocCombo = connect(m_combo, SIGNAL(currentIndexChanged(int)),
                         SLOT(handleIndexChanged(int)));
}

void
LauncherSelect::handleSettingChanged(const QVariant &value)
{
    int idx = m_combo->findData(value.toString());
    m_combo->setCurrentIndex(idx != -1 ? idx : 0);
}

void
LauncherSelect::handleClicked()
{
    if (g_launchwin == nullptr)
        g_launchwin = new LaunchersWindow();

    g_launchwin->bringUp();
}


QWidget *
LauncherSelectFactory::createWidget(const SettingDef *def, SettingsBase *settings) const
{
    return new LauncherSelect(def, settings);
}
