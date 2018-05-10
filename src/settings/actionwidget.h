// Copyright © 2018 TermySequence LLC
//
// SPDX-License-Identifier: GPL-2.0-only

#pragma once

#include "settingwidget.h"
#include <QWidget>

QT_BEGIN_NAMESPACE
class QPushButton;
QT_END_NAMESPACE
struct SettingDef;
class SettingsBase;

class ActionWidget final: public QWidget
{
    Q_OBJECT

private:
    QPushButton *m_button;

    const char *m_actionSlot;
    SettingsBase *m_settings;

private slots:
    void handleClicked();

public:
    ActionWidget(const char *actionSlot, const char *label, SettingsBase *settings);
};


class ActionWidgetFactory final: public SettingWidgetFactory
{
private:
    const char *m_actionSlot, *m_label;

public:
    ActionWidgetFactory(const char *actionSlot, const char *label);

    QWidget* createWidget(const SettingDef *def, SettingsBase *settings) const;
};
