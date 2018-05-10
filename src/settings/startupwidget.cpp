// Copyright © 2018 TermySequence LLC
//
// SPDX-License-Identifier: GPL-2.0-only

#include "common.h"
#include "app/attrbase.h"
#include "startupwidget.h"
#include "startupdialog.h"

#include <QLabel>
#include <QPushButton>
#include <QHBoxLayout>

#define TR_BUTTON1 TL("input-button", "Choose") + A("...")
#define TR_TEXT1 TL("window-text", "%n Terminal(s)", "", n)

StartupWidget::StartupWidget(const SettingDef *def, SettingsBase *settings) :
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

    connect(button, SIGNAL(clicked()), SLOT(handleClicked()));
}

void
StartupWidget::handleClicked()
{
    if (!m_dialog)
        m_dialog = new StartupDialog(m_def, m_settings, this);

    m_dialog->bringUp();
    m_dialog->exec();
}

void
StartupWidget::handleSettingChanged(const QVariant &value)
{
    int n = value.toStringList().size();
    m_label->setText(TR_TEXT1);
}


QWidget *
StartupWidgetFactory::createWidget(const SettingDef *def, SettingsBase *settings) const
{
    return new StartupWidget(def, settings);
}
