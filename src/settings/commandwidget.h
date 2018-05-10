// Copyright © 2018 TermySequence LLC
//
// SPDX-License-Identifier: GPL-2.0-only

#pragma once

#include "settingwidget.h"

QT_BEGIN_NAMESPACE
class QLineEdit;
QT_END_NAMESPACE
class CommandDialog;

class CommandWidget final: public SettingWidget
{
    Q_OBJECT

private:
    QLineEdit *m_text;
    CommandDialog *m_dialog = nullptr;

private slots:
    void handleTextEdited();
    void handleClicked();

protected:
    void handleSettingChanged(const QVariant &value);

public:
    CommandWidget(const SettingDef *def, SettingsBase *settings);
};


class CommandWidgetFactory final: public StringListWidgetFactory
{
public:
    QWidget* createWidget(const SettingDef *def, SettingsBase *settings) const;
};
