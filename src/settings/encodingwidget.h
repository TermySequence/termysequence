// Copyright © 2019 TermySequence LLC
//
// SPDX-License-Identifier: GPL-2.0-only

#pragma once

#include "settingwidget.h"

QT_BEGIN_NAMESPACE
class QLabel;
QT_END_NAMESPACE
class EncodingDialog;

class EncodingWidget final: public SettingWidget
{
    Q_OBJECT

private:
    QLabel *m_label;
    EncodingDialog *m_dialog = nullptr;

private slots:
    void handleClicked();

protected:
    void handleSettingChanged(const QVariant &value);

public:
    EncodingWidget(const SettingDef *def, SettingsBase *settings);
};


class EncodingWidgetFactory final: public StringListWidgetFactory
{
public:
    QWidget* createWidget(const SettingDef *def, SettingsBase *settings) const;
};
