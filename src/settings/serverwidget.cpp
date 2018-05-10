// Copyright © 2018 TermySequence LLC
//
// SPDX-License-Identifier: GPL-2.0-only

#include "common.h"
#include "app/attrbase.h"
#include "serverwidget.h"
#include "settings.h"
#include "servinfo.h"

#include <QComboBox>
#include <QHBoxLayout>

#define TR_NAME1 TL("settings-name", "Connect directly from client")

//
// Server combo
//
ServerCombo::ServerCombo(QWidget *parent) :
    QComboBox(parent)
{
    addItem(TR_NAME1);
    insertSeparator(1);

    for (auto *info: g_settings->servers()) {
        info->activate();
        addItem(info->nameIcon(), info->fullname(), info->idStr());
    }

    connect(g_settings, SIGNAL(serverAdded()), SLOT(handleServerAdded()));
    connect(g_settings, SIGNAL(serverUpdated(int)), SLOT(handleServerUpdated(int)));
}

void
ServerCombo::handleServerAdded()
{
    auto *info = g_settings->servers().back();
    addItem(info->nameIcon(), info->fullname(), info->idStr());
}

void
ServerCombo::handleServerUpdated(int index)
{
    auto *info = g_settings->servers()[index];
    int row = findData(info->idStr());
    if (row != -1) {
        setItemText(row, info->fullname());
        setItemIcon(row, info->nameIcon());
    }
}

//
// Server widget
//
ServerWidget::ServerWidget(const SettingDef *def, SettingsBase *settings) :
    SettingWidget(def, settings)
{
    m_combo = new ServerCombo;
    m_combo->installEventFilter(this);

    QHBoxLayout *layout = new QHBoxLayout;
    layout->setContentsMargins(g_mtmargins);
    layout->addWidget(m_combo);
    setLayout(layout);

    handleSettingChanged(m_value);

    connect(m_combo, SIGNAL(currentIndexChanged(int)), this, SLOT(handleIndexChanged()));
}

void
ServerWidget::handleIndexChanged()
{
    setProperty(m_combo->currentData());
}

void
ServerWidget::handleSettingChanged(const QVariant &value)
{
    int row = m_combo->findData(value);
    if (row != -1)
        m_combo->setCurrentIndex(row);
}


QWidget *
ServerWidgetFactory::createWidget(const SettingDef *def, SettingsBase *settings) const
{
    return new ServerWidget(def, settings);
}
