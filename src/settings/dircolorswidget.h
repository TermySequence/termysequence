// Copyright © 2018 TermySequence LLC
//
// SPDX-License-Identifier: GPL-2.0-only

#pragma once

#include "settingwidget.h"

class TermPalette;

class DircolorsWidget final: public SettingWidget
{
    Q_OBJECT

private slots:
    void handleClicked();

protected:
    void handleSettingChanged(const QVariant &value);

public:
    DircolorsWidget(const SettingDef *def, SettingsBase *settings);
};


class DircolorsWidgetFactory final: public SettingWidgetFactory
{
public:
    QWidget* createWidget(const SettingDef *def, SettingsBase *settings) const;
};
