// Copyright © 2018 TermySequence LLC
//
// SPDX-License-Identifier: GPL-2.0-only

#pragma once

#include "checkwidget.h"

class EncodingCheck final: public CheckWidget
{
    Q_OBJECT

private:
    QString m_paramOn;
    QString m_paramOff;

private slots:
    void handleStateChanged(int state);

protected:
    void handleSettingChanged(const QVariant &value);

public:
    EncodingCheck(const SettingDef *def, SettingsBase *settings,
                  const char *param);
};


class EncodingCheckFactory final: public SettingWidgetFactory
{
private:
    const char *m_param;

public:
    EncodingCheckFactory(const char *param);

    QWidget* createWidget(const SettingDef *def, SettingsBase *settings) const;
};
