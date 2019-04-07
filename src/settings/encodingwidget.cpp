// Copyright © 2019 TermySequence LLC
//
// SPDX-License-Identifier: GPL-2.0-only

#include "common.h"
#include "app/attrbase.h"
#include "encodingwidget.h"
#include "encodingdialog.h"
#include "base.h"

#include <QLabel>
#include <QPushButton>
#include <QHBoxLayout>

#define TR_BUTTON1 TL("input-button", "Configure") + A("...")
#define TR_TEXT1 TL("window-text", "%n Parameter(s)", "", n)

EncodingWidget::EncodingWidget(const SettingDef *def, SettingsBase *settings) :
    SettingWidget(def, settings)
{
    m_label = new QLabel;

    QPushButton *button = new QPushButton(TR_BUTTON1);

    QHBoxLayout *layout = new QHBoxLayout;
    layout->setContentsMargins(g_mtmargins);
    layout->addWidget(button);
    layout->addWidget(m_label, 1);
    setLayout(layout);

    handleSettingChanged(m_value);

    connect(button, SIGNAL(clicked()), this, SLOT(handleClicked()));
}

void
EncodingWidget::handleClicked()
{
    if (!m_dialog)
        m_dialog = new EncodingDialog(m_def, m_settings, this);

    m_dialog->setContent(m_value.toStringList());
    m_dialog->exec();
}

void
EncodingWidget::handleSettingChanged(const QVariant &value)
{
    int n = value.toStringList().size() - 1;
    m_label->setText(TR_TEXT1);
}


QWidget *
EncodingWidgetFactory::createWidget(const SettingDef *def, SettingsBase *settings) const
{
    // Decrement the SettingDef pointer until its property is non-null
    // This setting corresponds to the encoding property that we will use
    while (def->property == nullptr)
        --def;

    return new EncodingWidget(def, settings);
}
