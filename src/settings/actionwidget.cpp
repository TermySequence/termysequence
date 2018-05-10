// Copyright © 2018 TermySequence LLC
//
// SPDX-License-Identifier: GPL-2.0-only

#include "common.h"
#include "actionwidget.h"
#include "base.h"

#include <QPushButton>
#include <QHBoxLayout>
#include <QCoreApplication>

ActionWidget::ActionWidget(const char *actionSlot, const char *label, SettingsBase *settings) :
    m_actionSlot(actionSlot),
    m_settings(settings)
{
    QString str(QCoreApplication::translate("input-button", label));
    m_button = new QPushButton(str);

    QHBoxLayout *layout = new QHBoxLayout;
    layout->addWidget(m_button);
    layout->addStretch(1);
    setLayout(layout);

    connect(m_button, SIGNAL(clicked()), SLOT(handleClicked()));
}

void
ActionWidget::handleClicked()
{
    QMetaObject::invokeMethod(m_settings, m_actionSlot, Qt::DirectConnection);
}


ActionWidgetFactory::ActionWidgetFactory(const char *actionSlot, const char *label) :
    m_actionSlot(actionSlot),
    m_label(label)
{
}

QWidget *
ActionWidgetFactory::createWidget(const SettingDef *def, SettingsBase *settings) const
{
    return new ActionWidget(m_actionSlot, m_label, settings);
}
