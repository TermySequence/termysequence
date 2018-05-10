// Copyright © 2018 TermySequence LLC
//
// SPDX-License-Identifier: GPL-2.0-only

#pragma once

#include "settingwidget.h"

QT_BEGIN_NAMESPACE
class QPushButton;
class QLineEdit;
QT_END_NAMESPACE
class FormatDialog;
struct FormatDef;

class FormatWidget final: public SettingWidget
{
    Q_OBJECT

private:
    const FormatDef *m_specs;
    QPushButton *m_button;
    QLineEdit *m_text;
    FormatDialog *m_dialog;

private slots:
    void handleTextChanged();
    void handleClicked();

protected:
    void handleSettingChanged(const QVariant &value);

public:
    FormatWidget(const SettingDef *def, SettingsBase *settings, const FormatDef *specs);
};


class FormatWidgetFactory final: public SettingWidgetFactory
{
private:
    const FormatDef *m_specs;

public:
    FormatWidgetFactory(const FormatDef *specs);

    QWidget* createWidget(const SettingDef *def, SettingsBase *settings) const;
};
