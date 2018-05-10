// Copyright © 2018 TermySequence LLC
//
// SPDX-License-Identifier: GPL-2.0-only

#pragma once

#include "settingwidget.h"

QT_BEGIN_NAMESPACE
class QLabel;
QT_END_NAMESPACE
class StartupDialog;

class StartupWidget final: public SettingWidget
{
    Q_OBJECT

private:
    QLabel *m_label;
    StartupDialog *m_dialog = nullptr;

private slots:
    void handleClicked();

protected:
    void handleSettingChanged(const QVariant &value);

public:
    StartupWidget(const SettingDef *def, SettingsBase *settings);
};


class StartupWidgetFactory final: public SettingWidgetFactory
{
public:
    QWidget* createWidget(const SettingDef *def, SettingsBase *settings) const;
};
