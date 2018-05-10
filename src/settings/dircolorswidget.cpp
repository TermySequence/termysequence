// Copyright © 2018 TermySequence LLC
//
// SPDX-License-Identifier: GPL-2.0-only

#include "common.h"
#include "app/attrbase.h"
#include "dircolorswidget.h"
#include "dircolorsdialog.h"

#include <QPushButton>
#include <QHBoxLayout>
#include <QFontDatabase>

#define TR_BUTTON1 TL("input-button", "Dircolors") + A("...")

DircolorsWidget::DircolorsWidget(const SettingDef *def, SettingsBase *settings) :
    SettingWidget(def, settings)
{
    QPushButton *button = new QPushButton(TR_BUTTON1);

    QHBoxLayout *layout = new QHBoxLayout;
    layout->setContentsMargins(g_mtmargins);
    layout->addWidget(button);
    layout->addStretch(1);
    setLayout(layout);

    connect(button, SIGNAL(clicked()), SLOT(handleClicked()));
}

void
DircolorsWidget::handleClicked()
{
    QFont font;
    if (!font.fromString(getOther("font").toString()))
        font = QFontDatabase::systemFont(QFontDatabase::FixedFont);

    TermPalette scratch(getOther("palette").toString(), m_value.toString());

    auto *dialog = new DircolorsDialog(scratch, font, this);
    connect(dialog, &DircolorsDialog::modified, [=]{
        setProperty(dialog->dircolors().dStr());
    });
    connect(dialog, &QDialog::rejected, [=]{
        setProperty(dialog->saved().dStr());
    });
    dialog->bringUp();
}

void
DircolorsWidget::handleSettingChanged(const QVariant &value)
{
    // Note: not updating dialog in response to external change
}


QWidget *
DircolorsWidgetFactory::createWidget(const SettingDef *def, SettingsBase *settings) const
{
    return new DircolorsWidget(def, settings);
}
