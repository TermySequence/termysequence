// Copyright © 2018 TermySequence LLC
//
// SPDX-License-Identifier: GPL-2.0-only

#pragma once

#include "settingwidget.h"

QT_BEGIN_NAMESPACE
class QComboBox;
QT_END_NAMESPACE

class KeymapSelect final: public SettingWidget
{
    Q_OBJECT

private:
    QComboBox *m_combo;
    QMetaObject::Connection m_mocCombo;

    void populateValues();

private slots:
    void handleIndexChanged();
    void handleKeymapsChanged();
    void handleClicked();

protected:
    void handleSettingChanged(const QVariant &value);

public:
    KeymapSelect(const SettingDef *def, SettingsBase *settings);
};


class KeymapSelectFactory final: public SettingWidgetFactory
{
public:
    QWidget* createWidget(const SettingDef *def, SettingsBase *settings) const;
};
