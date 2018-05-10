// Copyright © 2018 TermySequence LLC
//
// SPDX-License-Identifier: GPL-2.0-only

#include "common.h"
#include "app/attrbase.h"
#include "dialog.h"
#include "settings.h"

#include <QCheckBox>
#include <QDialogButtonBox>
#include <QVBoxLayout>
#include <QGridLayout>

#define OBJPROP_SIZE "opSize"

SettingsDialog::SettingsDialog(QWidget *parent) :
    QDialog(parent),
    m_sizes(g_settings->sizes())
{
    setWindowTitle(A("Settings"));
    setAttribute(Qt::WA_DeleteOnClose, true);
    setWindowModality(Qt::NonModal);
    setSizeGripEnabled(true);

    auto *layout = new QGridLayout;
    for (int i = 0; IconSettings::allSizes[i]; ++i) {
        int size = IconSettings::allSizes[i];
        auto *checkbox = new QCheckBox(QString::number(size));
        checkbox->setProperty(OBJPROP_SIZE, size);
        checkbox->setChecked(m_sizes.contains(size));
        connect(checkbox, SIGNAL(toggled(bool)), SLOT(handleSizeChange(bool)));
        layout->addWidget(checkbox, i, 0);
    }

    auto *checkbox = new QCheckBox("Enable filtering");
    checkbox->setChecked(g_settings->filter());
    connect(checkbox, &QCheckBox::toggled, g_settings, &IconSettings::setFilter);
    layout->addWidget(checkbox, 0, 1);

    auto *buttonBox = new QDialogButtonBox(QDialogButtonBox::Close);
    auto *mainLayout = new QVBoxLayout;
    mainLayout->addLayout(layout, 1);
    mainLayout->addWidget(buttonBox);
    setLayout(mainLayout);

    connect(buttonBox, SIGNAL(rejected()), SLOT(reject()));
}

void
SettingsDialog::handleSizeChange(bool checked)
{
    int size = sender()->property(OBJPROP_SIZE).toInt();

    if (checked)
        m_sizes.insert(size);
    else
        m_sizes.remove(size);

    g_settings->setSizes(m_sizes);
}
