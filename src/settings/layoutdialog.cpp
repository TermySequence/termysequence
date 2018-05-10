// Copyright © 2018 TermySequence LLC
//
// SPDX-License-Identifier: GPL-2.0-only

#include "common.h"
#include "app/attrbase.h"
#include "layoutdialog.h"
#include "layouttabs.h"

#include <QPushButton>
#include <QDialogButtonBox>
#include <QVBoxLayout>

#define TR_TITLE1 TL("window-title", "Adjust Terminal Layout")

LayoutDialog::LayoutDialog(const TermLayout &saved, const Termcolors &tcpal,
                           const QFont &font, QWidget *parent) :
    QDialog(parent),
    m_layout(saved)
{
    setWindowTitle(TR_TITLE1);
    setAttribute(Qt::WA_DeleteOnClose, true);
    setWindowModality(Qt::WindowModal);
    setSizeGripEnabled(true);

    m_tabs = new LayoutTabs(m_layout, tcpal, font);

    QDialogButtonBox *buttonBox = new QDialogButtonBox(
        QDialogButtonBox::Ok|QDialogButtonBox::Cancel|QDialogButtonBox::RestoreDefaults);
    QPushButton *resetButton = buttonBox->button(QDialogButtonBox::RestoreDefaults);

    QVBoxLayout *layout = new QVBoxLayout;
    layout->addWidget(m_tabs, 1);
    layout->addWidget(buttonBox);
    setLayout(layout);

    connect(buttonBox, SIGNAL(accepted()), SLOT(accept()));
    connect(buttonBox, SIGNAL(rejected()), SLOT(reject()));
    connect(resetButton, SIGNAL(clicked()), SLOT(handleReset()));
}

void
LayoutDialog::handleReset()
{
    m_layout = TermLayout();
    emit m_tabs->modified();
    m_tabs->reloadData();
}
