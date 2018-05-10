// Copyright © 2018 TermySequence LLC
//
// SPDX-License-Identifier: GPL-2.0-only

#pragma once

#include "settingwidget.h"

QT_BEGIN_NAMESPACE
class QLabel;
QT_END_NAMESPACE

class LayoutWidget final: public SettingWidget
{
    Q_OBJECT

private:
    int m_type;
    QLabel *m_label;

private slots:
    void handleClicked();

protected:
    void handleSettingChanged(const QVariant &value);

public:
    LayoutWidget(const SettingDef *def, SettingsBase *settings, int type);
};


class LayoutWidgetFactory final: public SettingWidgetFactory
{
private:
    int m_type;

public:
    LayoutWidgetFactory(int type);

    QWidget* createWidget(const SettingDef *def, SettingsBase *settings) const;
};
