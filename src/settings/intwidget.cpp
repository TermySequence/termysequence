// Copyright © 2018 TermySequence LLC
//
// SPDX-License-Identifier: GPL-2.0-only

#include "common.h"
#include "app/attrbase.h"
#include "intwidget.h"

#include <QSpinBox>
#include <QHBoxLayout>

#define TR_DIM0 TL("settings-dimension", "Disabled", "value")
#define TR_DIM1 ' ' + TL("settings-dimension", "items")
#define TR_DIM2 ' ' + TL("settings-dimension", "prompts")
#define TR_DIM3 ' ' + TL("settings-dimension", "files")
#define TR_DIM4 TL("settings-dimension", "ms")
#define TR_DIM5 ' ' + TL("settings-dimension", "min")
#define TR_DIM6 ' ' + TL("settings-dimension", "blinks")
#define TR_DIM7 ' ' + TL("settings-dimension", "pt")
#define TR_DIM8 ' ' + TL("settings-dimension", "times normal")

IntWidget::IntWidget(const SettingDef *def, SettingsBase *settings,
                     Dimension dim, int min, int max, int step, bool option) :
    SettingWidget(def, settings)
{
    QString dimStr;
    switch (dim) {
    case Items:
        dimStr = TR_DIM1;
        break;
    case Prompts:
        dimStr = TR_DIM2;
        break;
    case Files:
        dimStr = TR_DIM3;
        break;
    case Millis:
        dimStr = TR_DIM4;
        break;
    case Minutes:
        dimStr = TR_DIM5;
        break;
    case Blinks:
        dimStr = TR_DIM6;
        break;
    case Points:
        dimStr = TR_DIM7;
        break;
    case Times:
        dimStr = TR_DIM8;
        break;
    }

    m_spin = new QSpinBox;
    m_spin->installEventFilter(this);
    m_spin->setRange(min, max);
    m_spin->setSingleStep(step);
    m_spin->setSuffix(dimStr);
    if (option)
        m_spin->setSpecialValueText(TR_DIM0);

    QHBoxLayout *layout = new QHBoxLayout;
    layout->setContentsMargins(g_mtmargins);
    layout->addWidget(m_spin);
    setLayout(layout);

    handleSettingChanged(m_value);
    connect(m_spin, SIGNAL(valueChanged(int)), this, SLOT(handleValueChanged(int)));
}

void
IntWidget::handleValueChanged(int value)
{
    setProperty(value);
}

void
IntWidget::handleSettingChanged(const QVariant &value)
{
    m_spin->setValue(value.toInt());
}


IntWidgetFactory::IntWidgetFactory(IntWidget::Dimension dim, int min, int max,
                                   int step, bool option) :
    m_dim(dim),
    m_min(min), m_max(max), m_step(step),
    m_option(option)
{
}

QWidget *
IntWidgetFactory::createWidget(const SettingDef *def, SettingsBase *settings) const
{
    return new IntWidget(def, settings, m_dim, m_min, m_max, m_step, m_option);
}
