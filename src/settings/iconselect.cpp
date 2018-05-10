// Copyright © 2018 TermySequence LLC
//
// SPDX-License-Identifier: GPL-2.0-only

#include "common.h"
#include "app/attrbase.h"
#include "iconselect.h"
#include "base/thumbicon.h"

#include <QComboBox>
#include <QHBoxLayout>

IconSelect::IconSelect(const SettingDef *def, SettingsBase *settings) :
    SettingWidget(def, settings)
{
    m_combo = new QComboBox;
    m_combo->installEventFilter(this);
    m_combo->addItems(ThumbIconCache::allThemes);
    m_combo->setCurrentText(m_value.toString());

    QHBoxLayout *layout = new QHBoxLayout;
    layout->setContentsMargins(g_mtmargins);
    layout->addWidget(m_combo);
    // layout->addStretch(1);
    setLayout(layout);

    connect(m_combo, SIGNAL(currentIndexChanged(int)), SLOT(handleIndexChanged()));
}

void
IconSelect::handleIndexChanged()
{
    setProperty(m_combo->currentText());
}

void
IconSelect::handleSettingChanged(const QVariant &)
{
    // do nothing
}


QWidget *
IconSelectFactory::createWidget(const SettingDef *def, SettingsBase *settings) const
{
    return new IconSelect(def, settings);
}
