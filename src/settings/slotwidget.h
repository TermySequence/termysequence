// Copyright © 2018 TermySequence LLC
//
// SPDX-License-Identifier: GPL-2.0-only

#pragma once

#include "settingwidget.h"

QT_BEGIN_NAMESPACE
class QComboBox;
QT_END_NAMESPACE

class SlotWidget final: public SettingWidget
{
    Q_OBJECT

private:
    QComboBox *m_combo;

private slots:
    void handleTextChanged(const QString &text);

protected:
    void handleSettingChanged(const QVariant &value);

public:
    SlotWidget(const SettingDef *def, SettingsBase *settings, const QStringList &list);
};


class SlotWidgetFactory final: public SettingWidgetFactory
{
private:
    bool m_termSlot;

public:
    SlotWidgetFactory(bool termSlot);

    QWidget* createWidget(const SettingDef *def, SettingsBase *settings) const;
};
