// Copyright © 2018 TermySequence LLC
//
// SPDX-License-Identifier: GPL-2.0-only

#include "common.h"
#include "app/attrbase.h"
#include "choicewidget.h"
#include "base/thumbicon.h"

#include <QComboBox>
#include <QHBoxLayout>

ChoiceWidget::ChoiceWidget(const SettingDef *def, SettingsBase *settings,
                           const ChoiceDef *choices) :
    SettingWidget(def, settings)
{
    m_combo = new QComboBox();
    m_combo->installEventFilter(this);

    for (const ChoiceDef *choice = choices; choice->description; ++choice) {
        QString str(QCoreApplication::translate("settings-enum", choice->description));
        if (choice->icon)
            m_combo->addItem(ThumbIcon::fromTheme(choice->icon), str, choice->value);
        else
            m_combo->addItem(str, choice->value);
    }

    QHBoxLayout *layout = new QHBoxLayout;
    layout->setContentsMargins(g_mtmargins);
    layout->addWidget(m_combo);
    // layout->addStretch(1);
    setLayout(layout);

    handleSettingChanged(m_value);

    connect(m_combo, SIGNAL(currentIndexChanged(int)), this, SLOT(handleIndexChanged(int)));
}

void
ChoiceWidget::handleIndexChanged(int index)
{
    setProperty(m_combo->itemData(index));
}

void
ChoiceWidget::handleSettingChanged(const QVariant &value)
{
    int row = m_combo->findData(value);
    if (row != -1)
        m_combo->setCurrentIndex(row);
}

ChoiceWidgetFactory::ChoiceWidgetFactory(const ChoiceDef *choices) :
    m_choices(choices)
{
}

QWidget *
ChoiceWidgetFactory::createWidget(const SettingDef *def, SettingsBase *settings) const
{
    return new ChoiceWidget(def, settings, m_choices);
}
