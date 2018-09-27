// Copyright © 2018 TermySequence LLC
//
// SPDX-License-Identifier: GPL-2.0-only

#pragma once

#include "settingwidget.h"

#include <QComboBox>

//
// Server combo
//
class ServerCombo final: public QComboBox
{
    Q_OBJECT

private slots:
    void handleServerAdded();
    void handleServerUpdated(int index);

public:
    ServerCombo(QWidget *parent = nullptr);

    QString currentHost();
};

//
// Server widget
//
class ServerWidget final: public SettingWidget
{
    Q_OBJECT

protected:
    ServerCombo *m_combo;

private slots:
    virtual void handleIndexChanged();

protected:
    void handleSettingChanged(const QVariant &value);

public:
    ServerWidget(const SettingDef *def, SettingsBase *settings);
};


class ServerWidgetFactory final: public SettingWidgetFactory
{
public:
    QWidget* createWidget(const SettingDef *def, SettingsBase *settings) const;
};
