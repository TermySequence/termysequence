// Copyright © 2018 TermySequence LLC
//
// SPDX-License-Identifier: GPL-2.0-only

#include "common.h"
#include "app/attrbase.h"
#include "app/iconbutton.h"
#include "palettedialog.h"
#include "themetab.h"
#include "palettetab.h"
#include "colorstab.h"
#include "dircolorstab.h"
#include "global.h"

#include <QVBoxLayout>
#include <QTabWidget>

#define TR_TAB1 TL("tab-title", "Saved Themes")
#define TR_TAB2 TL("tab-title", "Basic Colors")
#define TR_TAB3 TL("tab-title", "Extended Colors")
#define TR_TAB4 TL("tab-title", "Dircolors")
#define TR_TITLE1 TL("window-title", "Edit Theme")

PaletteDialog::PaletteDialog(const TermPalette &palette, const QFont &font,
                             bool modal, QWidget *parent) :
    QDialog(parent),
    m_palette(palette),
    m_saved(palette)
{
    setWindowTitle(TR_TITLE1);
    setAttribute(Qt::WA_DeleteOnClose, true);
    setWindowModality(modal ? Qt::WindowModal : Qt::NonModal);
    setSizeGripEnabled(true);

    m_preTab = new ThemeTab(m_palette, m_saved, font);
    m_stdTab = new PaletteTab(m_palette);
    m_xtdTab = new ColorsTab(m_palette);
    m_dirTab = new DircolorsTab(m_palette, font);

    auto *buttonBox = new QDialogButtonBox(
        QDialogButtonBox::Ok|QDialogButtonBox::Cancel|QDialogButtonBox::Reset);
    buttonBox->addHelpButton("palette-editor");
    QPushButton *resetButton = buttonBox->button(QDialogButtonBox::Reset);

    m_tabs = new QTabWidget;
    m_tabs->addTab(m_preTab, TR_TAB1);
    m_tabs->addTab(m_stdTab, TR_TAB2);
    m_tabs->addTab(m_xtdTab, TR_TAB3);
    m_tabs->addTab(m_dirTab, TR_TAB4);

    QVBoxLayout *layout = new QVBoxLayout;
    layout->addWidget(m_tabs, 1);
    layout->addWidget(buttonBox);
    setLayout(layout);

    connect(m_preTab, SIGNAL(modified()), SLOT(reportTermcolorsModified()));
    connect(m_stdTab, SIGNAL(modified()), SLOT(reportTermcolorsModified()));
    connect(m_xtdTab, SIGNAL(modified()), SLOT(reportTermcolorsModified()));

    connect(m_preTab, SIGNAL(modified()), SLOT(reportDircolorsModified()));
    connect(m_dirTab, SIGNAL(modified()), SLOT(reportDircolorsModified()));

    connect(buttonBox, SIGNAL(accepted()), SLOT(accept()));
    connect(buttonBox, SIGNAL(rejected()), SLOT(reject()));
    connect(resetButton, SIGNAL(clicked()), SLOT(handleReset()));
}

void
PaletteDialog::reloadTermcolors(QObject *sender)
{
    if (sender != m_preTab) m_preTab->reload();
    if (sender != m_stdTab) m_stdTab->reload();
    if (sender != m_xtdTab) m_xtdTab->reload();
}

void
PaletteDialog::reloadDircolors(QObject *sender)
{
    if (sender != m_preTab) m_preTab->reload();
    if (sender != m_dirTab) m_dirTab->reload();
}

void
PaletteDialog::reportTermcolorsModified()
{
    reloadTermcolors(sender());
    emit termcolorsModified();
}

void
PaletteDialog::reportDircolorsModified()
{
    reloadDircolors(sender());
    emit dircolorsModified();
}

void
PaletteDialog::handleReset()
{
    m_palette = m_saved;
    reloadTermcolors();
    reloadDircolors();
    emit termcolorsModified();
    emit dircolorsModified();
}

void
PaletteDialog::bringUp()
{
    // Bring up tabs
    m_stdTab->bringUp();
    m_xtdTab->bringUp();

    m_tabs->setCurrentIndex(0);

    show();
}
