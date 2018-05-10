// Copyright © 2018 TermySequence LLC
//
// SPDX-License-Identifier: GPL-2.0-only

#pragma once

#include <QWidget>
#include <QVariant>

struct SettingDef;
class SettingsBase;

class SettingWidget: public QWidget
{
    Q_OBJECT

protected:
    const SettingDef *m_def;
    SettingsBase *m_settings;

    QVariant m_value;

    virtual void handleSettingChanged(const QVariant &value) = 0;

    void setProperty(const QVariant &value);
    QVariant getDefaultValue();
    QVariant getOther(const char *name);
    void setOther(const char *name, const QVariant &value);

private slots:
    void settingChanged(const char *property, QVariant value);

public:
    SettingWidget(const SettingDef *def, SettingsBase *settings);

    bool eventFilter(QObject *object, QEvent *event);
};


class SettingWidgetFactory
{
public:
    virtual QWidget* createWidget(const SettingDef *def, SettingsBase *settings) const = 0;
    virtual ~SettingWidgetFactory() = default;

    virtual QString toString(const QVariant &value) const;
    virtual QVariant fromString(const QString &str) const;
};


class StringListWidgetFactory: public SettingWidgetFactory
{
public:
    QString toString(const QVariant &value) const;
    QVariant fromString(const QString &str) const;
};
