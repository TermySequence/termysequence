// Copyright © 2018 TermySequence LLC
//
// SPDX-License-Identifier: GPL-2.0-only

#include "common.h"
#include "app/attrbase.h"
#include "inputwidget.h"

#include <QLineEdit>
#include <QHBoxLayout>

InputWidget::InputWidget(const SettingDef *def, SettingsBase *settings) :
    SettingWidget(def, settings)
{
    m_text = new QLineEdit();

    QHBoxLayout *layout = new QHBoxLayout;
    layout->setContentsMargins(g_mtmargins);
    layout->addWidget(m_text, 1);
    setLayout(layout);

    handleSettingChanged(m_value);

    connect(m_text, SIGNAL(editingFinished()), this, SLOT(handleTextChanged()));
}

void
InputWidget::handleTextChanged()
{
    setProperty(m_text->text());
}

void
InputWidget::handleSettingChanged(const QVariant &value)
{
    m_text->setText(value.toString());
}


QWidget *
InputWidgetFactory::createWidget(const SettingDef *def, SettingsBase *settings) const
{
    return new InputWidget(def, settings);
}
