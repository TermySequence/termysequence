// Copyright © 2018 TermySequence LLC
//
// SPDX-License-Identifier: GPL-2.0-only

#pragma once

#include "settingwidget.h"

QT_BEGIN_NAMESPACE
class QComboBox;
QT_END_NAMESPACE

class ProfileSelect final: public SettingWidget
{
    Q_OBJECT

private:
    QComboBox *m_combo;
    QMetaObject::Connection m_mocCombo;
    QString m_deftext;

    void populateValues();

private slots:
    void handleIndexChanged(int index);
    void handleProfilesChanged();
    void handleClicked();

protected:
    void handleSettingChanged(const QVariant &value);

public:
    ProfileSelect(const SettingDef *def, SettingsBase *settings,
                  bool serverDefault);
};


class ProfileSelectFactory final: public SettingWidgetFactory
{
private:
    bool m_serverDefault;

public:
    ProfileSelectFactory(bool serverDefault);

    QWidget* createWidget(const SettingDef *def, SettingsBase *settings) const;
};
