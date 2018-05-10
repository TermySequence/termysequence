// Copyright © 2018 TermySequence LLC
//
// SPDX-License-Identifier: GPL-2.0-only

#include "common.h"
#include "app/attrbase.h"
#include "app/enums.h"
#include "app/icons.h"
#include "connectnew.h"
#include "connect.h"
#include "choicewidget.h"
#include "base/thumbicon.h"

#include <QLabel>
#include <QPushButton>
#include <QComboBox>
#include <QDialogButtonBox>
#include <QVBoxLayout>

#define TR_FIELD1 TL("input-field", "Add new") + ':'
#define TR_FIELD2 TL("input-field", "Connection type") + ':'
#define TR_TITLE1 TL("window-title", "Select Connection Type")

NewConnectDialog::NewConnectDialog(QWidget *parent):
    QDialog(parent)
{
    setWindowTitle(TR_TITLE1);
    setWindowModality(Qt::WindowModal);

    auto *ptr = ConnectSettings::g_typeDesc - 1;
    m_batch = new QComboBox;
    m_batch->addItem(TL("settings-enum", "Connection"));
    m_batch->addItem(ThumbIcon::fromTheme(ptr->icon),
                     QCoreApplication::translate("settings-enum", ptr->description),
                     ptr->value);

    m_type = new QComboBox;
    for (++ptr; ptr->description; ++ptr) {
        TypeEntry ent;
        ent.icon = ThumbIcon::fromTheme(ptr->icon);
        ent.name = QCoreApplication::translate("settings-enum", ptr->description);
        ent.value = ptr->value;
        m_type->addItem(ent.icon, ent.name, ent.value);
        m_types.append(std::move(ent));
    }

    QDialogButtonBox *buttonBox = new QDialogButtonBox;
    buttonBox->addButton(QDialogButtonBox::Ok);
    buttonBox->addButton(QDialogButtonBox::Cancel);

    QVBoxLayout *layout = new QVBoxLayout;
    layout->addWidget(new QLabel(TR_FIELD1));
    layout->addWidget(m_batch);
    layout->addWidget(new QLabel(TR_FIELD2));
    layout->addWidget(m_type);
    layout->addWidget(buttonBox);
    setLayout(layout);

    connect(buttonBox, SIGNAL(accepted()), SLOT(accept()));
    connect(buttonBox, SIGNAL(rejected()), SLOT(reject()));
    connect(m_batch, SIGNAL(currentIndexChanged(int)), SLOT(handleBatchChanged(int)));
}

void
NewConnectDialog::handleBatchChanged(int index)
{
    m_type->clear();
    m_type->setEnabled(!index);

    if (!index)
        for (const auto &i: m_types)
            m_type->addItem(i.icon, i.name, i.value);
}

int
NewConnectDialog::type() const
{
    if (m_batch->currentIndex())
        return Tsqt::ConnectionBatch;
    else
        return m_type->currentData().toInt();
}
