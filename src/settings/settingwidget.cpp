// Copyright © 2018 TermySequence LLC
//
// SPDX-License-Identifier: GPL-2.0-only

#include "common.h"
#include "settingwidget.h"
#include "base.h"

#include <QEvent>

SettingWidget::SettingWidget(const SettingDef *def, SettingsBase *settings) :
    m_def(def),
    m_settings(settings),
    m_value(settings->property(def->property))
{
    connect(settings, SIGNAL(settingChanged(const char*,QVariant)),
            SLOT(settingChanged(const char*,QVariant)));
}

void
SettingWidget::settingChanged(const char *property, QVariant value)
{
    if (strcmp(property, m_def->property) == 0)
        handleSettingChanged(m_value = value);
}

bool
SettingWidget::eventFilter(QObject *, QEvent *event)
{
    if (event->type() == QEvent::Wheel) {
        event->ignore();
        return true;
    }

    return false;
}

void
SettingWidget::setProperty(const QVariant &value)
{
    m_settings->setProperty(m_def->property, value);
}

QVariant
SettingWidget::getDefaultValue()
{
    return m_settings->defaultValue(m_def);
}

QVariant
SettingWidget::getOther(const char *name)
{
    return m_settings->property(name);
}

void
SettingWidget::setOther(const char *name, const QVariant &value)
{
    m_settings->setProperty(name, value);
}

QString
SettingWidgetFactory::toString(const QVariant &value) const
{
    return value.toString();
}

QVariant
SettingWidgetFactory::fromString(const QString &str) const
{
    return QVariant(str);
}


QString
StringListWidgetFactory::toString(const QVariant &value) const
{
    return value.toStringList().join('\x1f');
}

QVariant
StringListWidgetFactory::fromString(const QString &str) const
{
    return str.split('\x1f');
}
