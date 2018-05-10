// Copyright © 2018 TermySequence LLC
//
// SPDX-License-Identifier: GPL-2.0-only

#include "common.h"
#include "app/attrbase.h"
#include "powerwidget.h"

#include <QSpinBox>
#include <QHBoxLayout>
#include <QLabel>

#define TR_FIELD1 TL("input-field", "Power") + ':'
#define TR_FIELD2 TL("input-field", "Size") + ':'

PowerWidget::PowerWidget(const SettingDef *def, SettingsBase *settings, unsigned max) :
    SettingWidget(def, settings)
{
    m_spin = new QSpinBox;
    m_spin->installEventFilter(this);
    m_spin->setRange(0, max);
    m_label = new QLabel;

    QHBoxLayout *layout = new QHBoxLayout;
    layout->setContentsMargins(g_mtmargins);
    layout->addWidget(new QLabel(TR_FIELD1));
    layout->addWidget(m_spin);
    layout->addWidget(new QLabel(TR_FIELD2));
    layout->addWidget(m_label);
    layout->addStretch(1);
    setLayout(layout);

    handleSettingChanged(m_value);

    connect(m_spin, SIGNAL(valueChanged(int)), this, SLOT(handleValueChanged(int)));
}

void
PowerWidget::handleValueChanged(int value)
{
    setProperty(value);
}

void
PowerWidget::handleSettingChanged(const QVariant &value)
{
    unsigned power = value.toUInt();
    m_spin->setValue(power);
    m_label->setText(QString::number(1 << power));
}


PowerWidgetFactory::PowerWidgetFactory(unsigned max) : m_max(max)
{
}

QWidget *
PowerWidgetFactory::createWidget(const SettingDef *def, SettingsBase *settings) const
{
    return new PowerWidget(def, settings, m_max);
}
