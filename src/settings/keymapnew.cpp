// Copyright © 2018 TermySequence LLC
//
// SPDX-License-Identifier: GPL-2.0-only

#include "common.h"
#include "app/attrbase.h"
#include "keymapnew.h"
#include "keymap.h"
#include "settings.h"

#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QCheckBox>
#include <QComboBox>
#include <QDialogButtonBox>
#include <QVBoxLayout>

#define TR_CHECK1 TL("input-checkbox", "Inherit from keymap") + ':'
#define TR_FIELD1 TL("input-field", "New keymap name") + ':'
#define TR_TITLE1 TL("window-title", "New Keymap")

NewKeymapDialog::NewKeymapDialog(TermKeymap *keymap, bool cloning, QWidget *parent):
    QDialog(parent)
{
    setWindowTitle(TR_TITLE1);
    setWindowModality(Qt::WindowModal);

    m_name = new QLineEdit;
    m_inherit = new QCheckBox(TR_CHECK1);
    m_combo = new QComboBox;

    for (auto i: g_settings->keymaps())
        m_combo->addItem(i->name());

    if (keymap) {
        m_name->setText(keymap->name());
        if (keymap->parent()) {
            m_inherit->setChecked(true);
            m_combo->setCurrentText(keymap->parent()->name());
        }
    }

    m_inherit->setEnabled(!cloning);
    m_combo->setEnabled(!cloning && m_inherit->isChecked());

    QDialogButtonBox *buttonBox = new QDialogButtonBox;
    buttonBox->addButton(QDialogButtonBox::Ok);
    buttonBox->addButton(QDialogButtonBox::Cancel);

    QVBoxLayout *layout = new QVBoxLayout;
    layout->addWidget(new QLabel(TR_FIELD1));
    layout->addWidget(m_name);
    layout->addWidget(m_inherit);
    layout->addWidget(m_combo);
    layout->addWidget(buttonBox);
    setLayout(layout);

    connect(m_inherit, SIGNAL(stateChanged(int)), SLOT(handleInheritChanged()));
    connect(buttonBox, SIGNAL(accepted()), SLOT(accept()));
    connect(buttonBox, SIGNAL(rejected()), SLOT(reject()));
}

void
NewKeymapDialog::handleInheritChanged()
{
    m_combo->setEnabled(m_inherit->isChecked());
}

QString
NewKeymapDialog::name() const
{
    return m_name->text();
}

QString
NewKeymapDialog::parent() const
{
    return m_inherit->isChecked() ? m_combo->currentText() : g_mtstr;
}
