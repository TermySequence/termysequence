// Copyright © 2018 TermySequence LLC
//
// SPDX-License-Identifier: GPL-2.0-only

#include "common.h"
#include "app/attrbase.h"
#include "layoutwidget.h"
#include "layoutdialog.h"
#include "layouttabs.h"
#include "base.h"

#include <QLabel>
#include <QPushButton>
#include <QHBoxLayout>
#include <QFontDatabase>

#define TR_BUTTON1 TL("input-button", "Layout") + A("...")
#define TR_BUTTON2 TL("input-button", "Configure") + A("...")
#define TR_TEXT1 TL("window-text", "%n Fill(s)", "", n)

LayoutWidget::LayoutWidget(const SettingDef *def, SettingsBase *settings, int type) :
    SettingWidget(def, settings),
    m_type(type)
{
    QPushButton *button = new QPushButton(m_type ? TR_BUTTON2 : TR_BUTTON1);

    QHBoxLayout *layout = new QHBoxLayout;
    layout->setContentsMargins(g_mtmargins);
    layout->addWidget(button);

    if (m_type == 1) {
        layout->addWidget(m_label = new QLabel, 1);
        handleSettingChanged(m_value);
    } else {
        layout->addStretch(1);
    }

    setLayout(layout);

    connect(button, SIGNAL(clicked()), SLOT(handleClicked()));
}

void
LayoutWidget::handleClicked()
{
    Termcolors tcpal(getOther("palette").toString());
    QFont font;
    if (!font.fromString(getOther("font").toString()))
        font = QFontDatabase::systemFont(QFontDatabase::FixedFont);

    QString layoutStr, fillsStr, savedStr;
    if (m_type == 0) {
        savedStr = layoutStr = m_value.toString();
    } else {
        savedStr = fillsStr = m_value.toString();
    }
    TermLayout saved(layoutStr, fillsStr);

    auto *dialog = new LayoutDialog(saved, tcpal, font, this);
    auto *tabs = dialog->tabs();
    tabs->setExclusiveTab(m_type);

    connect(tabs, &LayoutTabs::modified, [=]{
        const auto &layout = dialog->layout();
        setProperty(m_type == 0 ? layout.layoutStr() : layout.fillsStr());
    });
    connect(dialog, &QDialog::rejected, [=]{
        setProperty(savedStr);
    });
    dialog->show();
}

void
LayoutWidget::handleSettingChanged(const QVariant &value)
{
    if (m_type == 1) {
        TermLayout layout;
        layout.parseFills(value.toString());
        int n = layout.fills().size();
        m_label->setText(TR_TEXT1);
    }
}


LayoutWidgetFactory::LayoutWidgetFactory(int type) :
    m_type(type)
{
}

QWidget *
LayoutWidgetFactory::createWidget(const SettingDef *def, SettingsBase *settings) const
{
    return new LayoutWidget(def, settings, m_type);
}
