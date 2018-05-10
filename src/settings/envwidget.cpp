// Copyright © 2018 TermySequence LLC
//
// SPDX-License-Identifier: GPL-2.0-only

#include "common.h"
#include "app/attrbase.h"
#include "envwidget.h"
#include "envdialog.h"

#include <QLabel>
#include <QPushButton>
#include <QHBoxLayout>

#define TR_BUTTON1 TL("input-button", "Environment") + A("...")
#define TR_TEXT1 TL("window-text", "%n Variable(s)", "", n)

EnvironWidget::EnvironWidget(const SettingDef *def, SettingsBase *settings) :
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
EnvironWidget::handleClicked()
{
    if (!m_dialog)
        m_dialog = new EnvironDialog(m_def, m_settings, this);

    m_dialog->setContent(m_value.toStringList());
    m_dialog->exec();
}

void
EnvironWidget::handleSettingChanged(const QVariant &value)
{
    int n = value.toStringList().size();
    m_label->setText(TR_TEXT1);
}


QWidget *
EnvironWidgetFactory::createWidget(const SettingDef *def, SettingsBase *settings) const
{
    return new EnvironWidget(def, settings);
}
