// Copyright © 2018 TermySequence LLC
//
// SPDX-License-Identifier: GPL-2.0-only

#pragma once

#include "settingwidget.h"

#include <QSize>

QT_BEGIN_NAMESPACE
class QSpinBox;
QT_END_NAMESPACE

class SizeWidget final: public SettingWidget
{
    Q_OBJECT

private:
    QSpinBox *m_cols, *m_rows;

    QSize m_size;

private slots:
    void handleColsChanged(int cols);
    void handleRowsChanged(int rows);

protected:
    void handleSettingChanged(const QVariant &value);

public:
    SizeWidget(const SettingDef *def, SettingsBase *settings);
};


class SizeWidgetFactory final: public SettingWidgetFactory
{
public:
    QWidget* createWidget(const SettingDef *def, SettingsBase *settings) const;

    QString toString(const QVariant &value) const;
    QVariant fromString(const QString &str) const;
};
