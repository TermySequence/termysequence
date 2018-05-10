// Copyright © 2018 TermySequence LLC
//
// SPDX-License-Identifier: GPL-2.0-only

#pragma once

#include "settingwidget.h"

QT_BEGIN_NAMESPACE
class QComboBox;
QT_END_NAMESPACE

class DownloadWidget final: public SettingWidget
{
    Q_OBJECT

private:
    QComboBox *m_combo;

    void addPath(int type, QString path);

private slots:
    void handleTextChanged(const QString &text);

protected:
    void handleSettingChanged(const QVariant &value);

public:
    DownloadWidget(const SettingDef *def, SettingsBase *settings, int type);
};


class DownloadWidgetFactory final: public SettingWidgetFactory
{
private:
    int m_type;

public:
    DownloadWidgetFactory(int type);

    QWidget* createWidget(const SettingDef *def, SettingsBase *settings) const;
};
