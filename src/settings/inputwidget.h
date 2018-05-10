// Copyright © 2018 TermySequence LLC
//
// SPDX-License-Identifier: GPL-2.0-only

#pragma once

#include "settingwidget.h"

QT_BEGIN_NAMESPACE
class QLineEdit;
QT_END_NAMESPACE

class InputWidget final: public SettingWidget
{
    Q_OBJECT

private:
    QLineEdit *m_text;

private slots:
    void handleTextChanged();

protected:
    void handleSettingChanged(const QVariant &value);

public:
    InputWidget(const SettingDef *def, SettingsBase *settings);
};


class InputWidgetFactory final: public SettingWidgetFactory
{
public:
    QWidget* createWidget(const SettingDef *def, SettingsBase *settings) const;
};
