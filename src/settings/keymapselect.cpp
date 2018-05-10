// Copyright © 2018 TermySequence LLC
//
// SPDX-License-Identifier: GPL-2.0-only

#include "common.h"
#include "app/attrbase.h"
#include "keymapselect.h"
#include "keymap.h"
#include "settings.h"
#include "keymapwindow.h"

#include <QPushButton>
#include <QComboBox>
#include <QHBoxLayout>
#include <algorithm>

#define TR_BUTTON1 TL("input-button", "Manage Keymaps") + A("...")

KeymapSelect::KeymapSelect(const SettingDef *def, SettingsBase *settings) :
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

    connect(g_settings, SIGNAL(keymapAdded()), SLOT(handleKeymapsChanged()));
    connect(g_settings, SIGNAL(keymapRemoved(int)), SLOT(handleKeymapsChanged()));
    connect(button, SIGNAL(clicked()), SLOT(handleClicked()));
    m_mocCombo = connect(m_combo, SIGNAL(currentIndexChanged(int)),
                         SLOT(handleIndexChanged()));
}

void
KeymapSelect::populateValues()
{
    m_combo->clear();

    for (auto i: g_settings->keymaps()) {
        m_combo->addItem(i->name());
    }

    handleSettingChanged(m_value);
}

void
KeymapSelect::handleKeymapsChanged()
{
    disconnect(m_mocCombo);
    populateValues();
    m_mocCombo = connect(m_combo, SIGNAL(currentIndexChanged(int)),
                         SLOT(handleIndexChanged()));
}

void
KeymapSelect::handleIndexChanged()
{
    setProperty(m_combo->currentText());
}

void
KeymapSelect::handleSettingChanged(const QVariant &value)
{
    m_combo->setCurrentText(value.toString());
}

void
KeymapSelect::handleClicked()
{
    if (g_keymapwin == nullptr)
        g_keymapwin = new KeymapsWindow();

    g_keymapwin->bringUp();
}

QWidget *
KeymapSelectFactory::createWidget(const SettingDef *def, SettingsBase *settings) const
{
    return new KeymapSelect(def, settings);
}
