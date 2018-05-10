// Copyright © 2018 TermySequence LLC
//
// SPDX-License-Identifier: GPL-2.0-only

#include "common.h"
#include "app/attrbase.h"
#include "formatwidget.h"
#include "formatdialog.h"

#include <QLineEdit>
#include <QPushButton>
#include <QHBoxLayout>

#define TR_BUTTON1 TL("input-button", "Edit") + A("...")

FormatWidget::FormatWidget(const SettingDef *def, SettingsBase *settings, const FormatDef *specs) :
    SettingWidget(def, settings),
    m_specs(specs)
{
    m_button = new QPushButton(TR_BUTTON1);
    m_text = new QLineEdit;
    m_text->setMaxLength(128);

    QHBoxLayout *layout = new QHBoxLayout;
    layout->setContentsMargins(g_mtmargins);
    layout->addWidget(m_text, 1);
    layout->addWidget(m_button);
    setLayout(layout);

    handleSettingChanged(m_value);

    connect(m_text, SIGNAL(editingFinished()), this, SLOT(handleTextChanged()));
    connect(m_button, SIGNAL(clicked()), this, SLOT(handleClicked()));
}

void
FormatWidget::handleTextChanged()
{
    setProperty(m_text->text());
}

void
FormatWidget::handleClicked()
{
    if (!m_dialog) {
        QString defval = getDefaultValue().toString();
        m_dialog = new FormatDialog(m_specs, defval, this);
    }

    m_dialog->setText(m_text->text());

    if (m_dialog->exec() == QDialog::Accepted)
        setProperty(m_dialog->text());
}

void
FormatWidget::handleSettingChanged(const QVariant &value)
{
    m_text->setText(value.toString());
}


FormatWidgetFactory::FormatWidgetFactory(const FormatDef *specs): m_specs(specs)
{
}

QWidget *
FormatWidgetFactory::createWidget(const SettingDef *def, SettingsBase *settings) const
{
    return new FormatWidget(def, settings, m_specs);
}
