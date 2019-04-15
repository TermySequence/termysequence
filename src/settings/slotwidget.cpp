// Copyright © 2018 TermySequence LLC
//
// SPDX-License-Identifier: GPL-2.0-only

#include "common.h"
#include "app/attrbase.h"
#include "app/slotcombo.h"
#include "slotwidget.h"
#include "base/manager.h"

#include <QHBoxLayout>

SlotWidget::SlotWidget(const SettingDef *def, SettingsBase *settings,
                       const QStringList &list) :
    SettingWidget(def, settings)
{
    m_combo = new SlotCombo;
    m_combo->installEventFilter(this);
    m_combo->addItems(list);

    QHBoxLayout *layout = new QHBoxLayout;
    layout->setContentsMargins(g_mtmargins);
    layout->addWidget(m_combo, 1);
    setLayout(layout);

    handleSettingChanged(m_value);

    connect(m_combo,
            SIGNAL(currentTextChanged(const QString&)),
            SLOT(handleTextChanged(const QString&)));
}

void
SlotWidget::handleTextChanged(const QString &text)
{
    setProperty(text);
}

void
SlotWidget::handleSettingChanged(const QVariant &value)
{
    QString text = value.toString();

    int row = m_combo->findText(text);
    if (row != -1) {
        m_combo->setCurrentIndex(row);
        return;
    }

    m_combo->setEditText(text);
}


SlotWidgetFactory::SlotWidgetFactory(bool termSlot) :
    m_termSlot(termSlot)
{
}

QWidget *
SlotWidgetFactory::createWidget(const SettingDef *def, SettingsBase *settings) const
{
    return new SlotWidget(def, settings, m_termSlot ?
                          TermManager::termSlots() :
                          TermManager::serverSlots());
}
