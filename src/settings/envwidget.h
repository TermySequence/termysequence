// Copyright © 2018 TermySequence LLC
//
// SPDX-License-Identifier: GPL-2.0-only

#pragma once

#include "settingwidget.h"

QT_BEGIN_NAMESPACE
class QLabel;
QT_END_NAMESPACE
class EnvironDialog;

class EnvironWidget final: public SettingWidget
{
    Q_OBJECT

private:
    QLabel *m_label;
    EnvironDialog *m_dialog = nullptr;

private slots:
    void handleClicked();

protected:
    void handleSettingChanged(const QVariant &value);

public:
    EnvironWidget(const SettingDef *def, SettingsBase *settings);
};


class EnvironWidgetFactory final: public StringListWidgetFactory
{
public:
    QWidget* createWidget(const SettingDef *def, SettingsBase *settings) const;
};
