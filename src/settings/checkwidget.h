// Copyright © 2018 TermySequence LLC
//
// SPDX-License-Identifier: GPL-2.0-only

#pragma once

#include "settingwidget.h"

QT_BEGIN_NAMESPACE
class QCheckBox;
QT_END_NAMESPACE

class CheckWidget: public SettingWidget
{
    Q_OBJECT

protected:
    QCheckBox *m_check;

private:
    uint64_t m_checkedValue;

private slots:
    virtual void handleStateChanged(int state);

protected:
    void handleSettingChanged(const QVariant &value);

public:
    CheckWidget(const SettingDef *def, SettingsBase *settings, uint64_t checkedValue);
};


class CheckWidgetFactory final: public SettingWidgetFactory
{
private:
    uint64_t m_checkedValue;

public:
    CheckWidgetFactory(uint64_t checkedValue = 0l);

    QWidget* createWidget(const SettingDef *def, SettingsBase *settings) const;
};
