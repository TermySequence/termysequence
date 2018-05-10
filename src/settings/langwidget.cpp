// Copyright © 2018 TermySequence LLC
//
// SPDX-License-Identifier: GPL-2.0-only

#include "common.h"
#include "app/attr.h"
#include "langwidget.h"

#include <QComboBox>
#include <QLineEdit>
#include <QHBoxLayout>

#define TR_SETTING1 TL("settings-enum", "Use client language setting")
#define TR_SETTING2 TL("settings-enum", "Use server language setting")
#define TR_SETTING3 TL("settings-enum", "Use custom locale") + ':'

LangWidget::LangWidget(const SettingDef *def, SettingsBase *settings) :
    SettingWidget(def, settings)
{
    m_combo = new QComboBox;
    m_combo->installEventFilter(this);
    m_text = new QLineEdit;

    m_combo->addItem(TR_SETTING1);
    m_combo->addItem(TR_SETTING2);
    m_combo->addItem(TR_SETTING3);

    QHBoxLayout *layout = new QHBoxLayout;
    layout->setContentsMargins(g_mtmargins);
    layout->addWidget(m_combo);
    layout->addWidget(m_text, 1);
    setLayout(layout);

    handleSettingChanged(m_value);

    connect(m_combo, SIGNAL(currentIndexChanged(int)), this, SLOT(handleIndexChanged(int)));
    connect(m_text, SIGNAL(editingFinished()), this, SLOT(handleTextChanged()));
}

void
LangWidget::handleIndexChanged(int index)
{
    QString val;

    switch (index) {
    case 0:
        m_text->setEnabled(false);
        m_text->clear();
        break;
    case 1:
        m_text->setEnabled(false);
        m_text->clear();
        val = g_str_CURRENT_PROFILE;
        break;
    default:
        m_text->setEnabled(true);
        val = getenv("LANG");
        m_text->setText(val);
        m_text->setFocus(Qt::OtherFocusReason);
    }

    setProperty(val);
}

void
LangWidget::handleTextChanged()
{
    if (m_combo->currentIndex() == 2)
        setProperty(m_text->text());
}

void
LangWidget::handleSettingChanged(const QVariant &value)
{
    QString str = value.toString();
    if (str.isEmpty()) {
        m_combo->setCurrentIndex(0);
        m_text->setEnabled(false);
        m_text->clear();
    }
    else if (str == g_str_CURRENT_PROFILE) {
        m_combo->setCurrentIndex(1);
        m_text->setEnabled(false);
        m_text->clear();
    }
    else {
        m_combo->setCurrentIndex(2);
        m_text->setEnabled(true);
        m_text->setText(str);
    }
}


QWidget *
LangWidgetFactory::createWidget(const SettingDef *def, SettingsBase *settings) const
{
    return new LangWidget(def, settings);
}
