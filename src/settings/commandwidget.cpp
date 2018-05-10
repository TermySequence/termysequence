// Copyright © 2018 TermySequence LLC
//
// SPDX-License-Identifier: GPL-2.0-only

#include "common.h"
#include "app/attrbase.h"
#include "app/iconbutton.h"
#include "commandwidget.h"
#include "commanddialog.h"
#include "launcher.h"

#include <QLineEdit>
#include <QHBoxLayout>
#include <QRegularExpression>

#define TR_BUTTON1 TL("input-button", "Advanced") + A("...")

CommandWidget::CommandWidget(const SettingDef *def, SettingsBase *settings) :
    SettingWidget(def, settings)
{
    m_text = new QLineEdit;

    QPushButton *button = new IconButton(ICON_EDIT_ITEM, TR_BUTTON1);

    QHBoxLayout *layout = new QHBoxLayout;
    layout->setContentsMargins(g_mtmargins);
    layout->addWidget(m_text, 1);
    layout->addWidget(button);
    setLayout(layout);

    handleSettingChanged(m_value);

    connect(m_text, SIGNAL(editingFinished()), SLOT(handleTextEdited()));
    connect(button, SIGNAL(clicked()), SLOT(handleClicked()));
}

void
CommandWidget::handleTextEdited()
{
    QRegularExpression space(L("\\s+"));
    QRegularExpression bracket(L("(\\S+)\\[(\\S+)\\]"));
    QStringList list = m_text->text().split(space, QString::SkipEmptyParts);

    if (!list.isEmpty()) {
        auto match = bracket.match(list.at(0));
        if (match.hasMatch()) {
            list.removeAt(0);
            list.insert(0, match.captured(2));
            list.insert(0, match.captured(1));
        } else {
            list.insert(1, list.at(0));
        }
    }

    setProperty(list);
}

void
CommandWidget::handleClicked()
{
    if (!m_dialog)
        m_dialog = new CommandDialog(m_def, m_settings, this);

    m_dialog->setContent(m_value.toStringList());
    m_dialog->exec();
}

void
CommandWidget::handleSettingChanged(const QVariant &value)
{
    m_text->setText(LaunchSettings::makeCommandStr(value.toStringList()));
}


QWidget *
CommandWidgetFactory::createWidget(const SettingDef *def, SettingsBase *settings) const
{
    return new CommandWidget(def, settings);
}
