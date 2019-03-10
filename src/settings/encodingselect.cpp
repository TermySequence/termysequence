// Copyright © 2018 TermySequence LLC
//
// SPDX-License-Identifier: GPL-2.0-only

#include "common.h"
#include "encodingselect.h"
#include "lib/unicode.h"

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
    int i = m_combo->findText(value.toStringList().value(0));
    m_combo->setCurrentIndex(i >= 0 ? i : 0);
}

EncodingSelectFactory::~EncodingSelectFactory()
{
    delete [] m_choices;
}

QWidget *
EncodingSelectFactory::createWidget(const SettingDef *def, SettingsBase *settings) const
{
    if (!m_choices) {
        size_t n = 1;
        for (const auto &i: Tsq::Unicoding::plugins())
            for (auto ptr = i->variants; ptr->variant; ++ptr)
                ++n;

        m_choices = new ChoiceDef[n]{};

        n = 0;
        for (const auto &i: Tsq::Unicoding::plugins())
            for (auto ptr = i->variants; ptr->variant; ++ptr) {
                m_choices[n].description = ptr->variant;
                m_choices[n].value = QString(ptr->variant);
                ++n;
            }
    }
    return new EncodingSelect(def, settings, m_choices);
}
