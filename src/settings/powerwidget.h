// Copyright © 2018 TermySequence LLC
//
// SPDX-License-Identifier: GPL-2.0-only

#pragma once

#include "settingwidget.h"

QT_BEGIN_NAMESPACE
class QSpinBox;
class QLabel;
QT_END_NAMESPACE

class PowerWidget final: public SettingWidget
{
    Q_OBJECT

private:
    QSpinBox *m_spin;
    QLabel *m_label;

private slots:
    void handleValueChanged(int value);

protected:
    void handleSettingChanged(const QVariant &value);

public:
    PowerWidget(const SettingDef *def, SettingsBase *settings, unsigned max);
};


class PowerWidgetFactory final: public SettingWidgetFactory
{
private:
    unsigned m_max;

public:
    PowerWidgetFactory(unsigned max);

    QWidget* createWidget(const SettingDef *def, SettingsBase *settings) const;
};
