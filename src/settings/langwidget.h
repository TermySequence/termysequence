// Copyright © 2018 TermySequence LLC
//
// SPDX-License-Identifier: GPL-2.0-only

#pragma once

#include "settingwidget.h"

QT_BEGIN_NAMESPACE
class QComboBox;
class QLineEdit;
QT_END_NAMESPACE

class LangWidget final: public SettingWidget
{
    Q_OBJECT

protected:
    QComboBox *m_combo;
    QLineEdit *m_text;

private slots:
    void handleIndexChanged(int index);
    void handleTextChanged();

protected:
    void handleSettingChanged(const QVariant &value);

public:
    LangWidget(const SettingDef *def, SettingsBase *settings);
};


class LangWidgetFactory final: public SettingWidgetFactory
{
public:
    QWidget* createWidget(const SettingDef *def, SettingsBase *settings) const;
};
