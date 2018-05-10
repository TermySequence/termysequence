// Copyright © 2018 TermySequence LLC
//
// SPDX-License-Identifier: GPL-2.0-only

#pragma once

#include "settingwidget.h"

QT_BEGIN_NAMESPACE
class QSpinBox;
QT_END_NAMESPACE

class IntWidget final: public SettingWidget
{
    Q_OBJECT

public:
    enum Dimension {
        Items, Prompts, Files,
        Millis, Minutes, Blinks,
        Points, Times,
    };

private:
    QSpinBox *m_spin;

private slots:
    void handleValueChanged(int value);

protected:
    void handleSettingChanged(const QVariant &value);

public:
    IntWidget(const SettingDef *def, SettingsBase *settings,
              IntWidget::Dimension dim, int min, int max, int step, bool option);
};


class IntWidgetFactory final: public SettingWidgetFactory
{
private:
    IntWidget::Dimension m_dim;
    int m_min, m_max, m_step;
    bool m_option;

public:
    IntWidgetFactory(IntWidget::Dimension dim, int min, int max,
                     int step = 1, bool option = false);

    QWidget* createWidget(const SettingDef *def, SettingsBase *settings) const;
};
