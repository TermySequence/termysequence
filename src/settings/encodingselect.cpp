// Copyright © 2018 TermySequence LLC
//
// SPDX-License-Identifier: GPL-2.0-only

#include "common.h"
#include "encodingselect.h"

#include <QComboBox>

EncodingSelect::EncodingSelect(const SettingDef *def, SettingsBase *settings,
                               const ChoiceDef *choices) :
    ChoiceWidget(def, settings, choices)
{
}

void
EncodingSelect::handleIndexChanged(int index)
{
    QStringList parts = m_value.toStringList();
    parts[0] = m_combo->itemText(index);
    setProperty(parts);
}

void
EncodingSelect::handleSettingChanged(const QVariant &value)
{
    QString front = value.toStringList().front();
    for (int i = 0; i < m_combo->count(); ++i)
        if (m_combo->itemText(i) == front) {
            m_combo->setCurrentIndex(i);
            break;
        }
}

EncodingSelectFactory::EncodingSelectFactory(const ChoiceDef *choices) :
    m_choices(choices)
{
}

QWidget *
EncodingSelectFactory::createWidget(const SettingDef *def, SettingsBase *settings) const
{
    return new EncodingSelect(def, settings, m_choices);
}
