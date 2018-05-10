// Copyright © 2018 TermySequence LLC
//
// SPDX-License-Identifier: GPL-2.0-only

#include "common.h"
#include "checkwidget.h"

#include <QCheckBox>
#include <QHBoxLayout>

CheckWidget::CheckWidget(const SettingDef *def, SettingsBase *settings,
                         uint64_t checkedValue) :
    SettingWidget(def, settings),
    m_checkedValue(checkedValue)
{
    m_check = new QCheckBox();

    QHBoxLayout *layout = new QHBoxLayout;
    layout->addWidget(m_check);
    layout->addStretch(1);
    setLayout(layout);

    handleSettingChanged(m_value);

    connect(m_check, SIGNAL(stateChanged(int)), this, SLOT(handleStateChanged(int)));
}

void
CheckWidget::handleStateChanged(int index)
{
    if (m_checkedValue == 0)
        setProperty(m_check->isChecked());
    else
        setProperty((qulonglong)(m_check->isChecked() ? m_checkedValue : 0));
}

void
CheckWidget::handleSettingChanged(const QVariant &value)
{
    m_check->setChecked(value.toBool());
}


CheckWidgetFactory::CheckWidgetFactory(uint64_t checkedValue) :
    m_checkedValue(checkedValue)
{
}

QWidget *
CheckWidgetFactory::createWidget(const SettingDef *def, SettingsBase *settings) const
{
    return new CheckWidget(def, settings, m_checkedValue);
}
