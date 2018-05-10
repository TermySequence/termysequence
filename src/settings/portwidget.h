// Copyright © 2018 TermySequence LLC
//
// SPDX-License-Identifier: GPL-2.0-only

#pragma once

#include "settingwidget.h"

QT_BEGIN_NAMESPACE
class QLabel;
QT_END_NAMESPACE
class PortsDialog;
class PortEditModel;

class PortsWidget final: public SettingWidget
{
    Q_OBJECT

private:
    QLabel *m_label;
    PortsDialog *m_dialog;
    PortEditModel *m_model;

private slots:
    void handlePorts();
    void handleClicked();

protected:
    void handleSettingChanged(const QVariant &value);

public:
    PortsWidget(const SettingDef *def, SettingsBase *settings);
};


class PortsWidgetFactory final: public SettingWidgetFactory
{
public:
    QWidget* createWidget(const SettingDef *def, SettingsBase *settings) const;
};
