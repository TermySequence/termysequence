// Copyright © 2018 TermySequence LLC
//
// SPDX-License-Identifier: GPL-2.0-only

#include "common.h"
#include "app/attrbase.h"
#include "app/iconbutton.h"
#include "portwidget.h"
#include "portdialog.h"
#include "porteditmodel.h"

#include <QLabel>
#include <QHBoxLayout>

#define TR_BUTTON1 TL("input-button", "Ports") + A("...")
#define TR_TEXT1 TL("window-text", "%n Port(s)", "", n)
#define TR_TEXT2 TL("window-text", "%n Automatic", "", a)

PortsWidget::PortsWidget(const SettingDef *def, SettingsBase *settings) :
    SettingWidget(def, settings)
{
    m_dialog = new PortsDialog(this);
    m_model = m_dialog->model();

    m_label = new QLabel;
    QPushButton *button = new IconButton(ICON_EDIT_ITEM, TR_BUTTON1);

    QHBoxLayout *layout = new QHBoxLayout;
    layout->setContentsMargins(g_mtmargins);
    layout->addWidget(button);
    layout->addWidget(m_label, 1);
    setLayout(layout);

    handleSettingChanged(m_value);

    connect(m_model, SIGNAL(portsChanged()), SLOT(handlePorts()));
    connect(button, SIGNAL(clicked()), SLOT(handleClicked()));
}

void
PortsWidget::handlePorts()
{
    setProperty(m_model->portsList());
}

void
PortsWidget::handleClicked()
{
    QStringList saved = m_model->portsList();
    m_dialog->bringUp();

    if (m_dialog->exec() != QDialog::Accepted)
        setProperty(saved);
}

void
PortsWidget::handleSettingChanged(const QVariant &value)
{
    m_model->setPortsList(value.toStringList());

    int n = m_model->rowCount(), a = m_model->autoCount();
    QString text = TR_TEXT1;
    text += A(", ");
    text += TR_TEXT2;
    m_label->setText(text);
}


QWidget *
PortsWidgetFactory::createWidget(const SettingDef *def, SettingsBase *settings) const
{
    return new PortsWidget(def, settings);
}
