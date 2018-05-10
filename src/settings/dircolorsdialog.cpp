// Copyright © 2018 TermySequence LLC
//
// SPDX-License-Identifier: GPL-2.0-only

#include "common.h"
#include "app/attrbase.h"
#include "app/iconbutton.h"
#include "dircolorsdialog.h"
#include "dircolorstab.h"
#include "global.h"

#include <QVBoxLayout>

#define TR_TITLE1 TL("window-title", "Adjust Dircolors")

DircolorsDialog::DircolorsDialog(const TermPalette &palette, const QFont &font,
                                 QWidget *parent) :
    QDialog(parent),
    m_palette(palette)
{
    setWindowTitle(TR_TITLE1);
    setAttribute(Qt::WA_DeleteOnClose, true);
    setWindowModality(Qt::WindowModal);
    setSizeGripEnabled(true);

    m_arrange = new DircolorsTab(m_palette, font);

    QDialogButtonBox *buttonBox = new QDialogButtonBox(
        QDialogButtonBox::Ok|QDialogButtonBox::Cancel|QDialogButtonBox::Reset);
    buttonBox->addHelpButton("dircolors-editor");
    QPushButton *reset = buttonBox->button(QDialogButtonBox::Reset);

    QVBoxLayout *layout = new QVBoxLayout;
    layout->addWidget(m_arrange, 1);
    layout->addWidget(buttonBox);
    setLayout(layout);

    connect(m_arrange, SIGNAL(modified()), SIGNAL(modified()));

    connect(buttonBox, SIGNAL(accepted()), SLOT(accept()));
    connect(buttonBox, SIGNAL(rejected()), SLOT(reject()));
    connect(reset, SIGNAL(clicked()), SLOT(handleReset()));
}

void
DircolorsDialog::handleReset()
{
    m_palette.Dircolors::operator=(m_saved);
    m_arrange->reload();
    emit modified();
}

void
DircolorsDialog::bringUp()
{
    // Save the current palette
    m_saved = m_palette;
    show();
}
