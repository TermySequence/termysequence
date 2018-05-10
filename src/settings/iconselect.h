// Copyright © 2018 TermySequence LLC
//
// SPDX-License-Identifier: GPL-2.0-only

#pragma once

#include "settingwidget.h"

QT_BEGIN_NAMESPACE
class QComboBox;
QT_END_NAMESPACE

class IconSelect final: public SettingWidget
{
    Q_OBJECT

private:
    QComboBox *m_combo;

private slots:
    void handleIndexChanged();

protected:
    void handleSettingChanged(const QVariant &value);

public:
    IconSelect(const SettingDef *def, SettingsBase *settings);
};


class IconSelectFactory final: public SettingWidgetFactory
{
public:
    QWidget* createWidget(const SettingDef *def, SettingsBase *settings) const;
};
