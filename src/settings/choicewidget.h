// Copyright © 2018 TermySequence LLC
//
// SPDX-License-Identifier: GPL-2.0-only

#pragma once

#include "settingwidget.h"

QT_BEGIN_NAMESPACE
class QComboBox;
QT_END_NAMESPACE

struct ChoiceDef
{
    const char *description;
    QVariant value;
    const char *icon;
};

extern const ChoiceDef g_dropConfig[];
extern const ChoiceDef g_serverDropConfig[];

class ChoiceWidget: public SettingWidget
{
    Q_OBJECT

protected:
    QComboBox *m_combo;

private slots:
    virtual void handleIndexChanged(int index);

protected:
    void handleSettingChanged(const QVariant &value);

public:
    ChoiceWidget(const SettingDef *def, SettingsBase *settings,
                 const ChoiceDef *choices);
};


class ChoiceWidgetFactory final: public SettingWidgetFactory
{
private:
    const ChoiceDef *m_choices;

public:
    ChoiceWidgetFactory(const ChoiceDef *choices);

    QWidget* createWidget(const SettingDef *def, SettingsBase *settings) const;
};
