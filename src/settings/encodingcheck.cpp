// Copyright © 2018 TermySequence LLC
//
// SPDX-License-Identifier: GPL-2.0-only

#include "common.h"
#include "encodingcheck.h"
#include "base.h"

#include <QCheckBox>

EncodingCheck::EncodingCheck(const SettingDef *def, SettingsBase *settings,
                             const char *param) :
    CheckWidget(def, settings, 0l),
    m_param(param)
{
    handleSettingChanged(m_value);
}

void
EncodingCheck::handleStateChanged(int index)
{
    QStringList parts = m_value.toStringList();
    parts.removeAll(m_param);

    if (m_check->isChecked())
        parts.append(m_param);

    setProperty(parts);
}

void
EncodingCheck::handleSettingChanged(const QVariant &value)
{
    QStringList parts = value.toStringList();
    m_check->setChecked(parts.contains(m_param));
}


EncodingCheckFactory::EncodingCheckFactory(const char *param) :
    m_param(param)
{
}

QWidget *
EncodingCheckFactory::createWidget(const SettingDef *def, SettingsBase *settings) const
{
    // Decrement the SettingDef pointer until its property is non-null
    // This setting corresponds to the encoding property that we will use
    while (def->property == nullptr)
        --def;

    return new EncodingCheck(def, settings, m_param);
}
