// Copyright © 2018 TermySequence LLC
//
// SPDX-License-Identifier: GPL-2.0-only

#pragma once

#include "choicewidget.h"

class EncodingSelect final: public ChoiceWidget
{
    Q_OBJECT

private slots:
    void handleIndexChanged(int index);

protected:
    void handleSettingChanged(const QVariant &value);

public:
    EncodingSelect(const SettingDef *def, SettingsBase *settings,
                   const ChoiceDef *choices);
};


class EncodingSelectFactory final: public StringListWidgetFactory
{
private:
    mutable ChoiceDef *m_choices = nullptr;

public:
    ~EncodingSelectFactory();

    QWidget* createWidget(const SettingDef *def, SettingsBase *settings) const;
};
