// Copyright © 2018 TermySequence LLC
//
// SPDX-License-Identifier: GPL-2.0-only

#include "common.h"
#include "app/config.h"
#include "base/listener.h"
#include "base/manager.h"
#include "base/term.h"
#include "powerdialog.h"
#include "profile.h"

#include <QSpinBox>
#include <QLabel>
#include <QCheckBox>
#include <QDialogButtonBox>
#include <QHBoxLayout>
#include <QVBoxLayout>

#define TR_FIELD1 TL("input-field", "Power") + ':'
#define TR_FIELD2 TL("input-field", "Scrollback Size") + ':'
#define TR_TITLE1 TL("window-title", "Adjust Scrollback Size")

PowerDialog::PowerDialog(TermInstance *term, TermManager *manager, QWidget *parent) :
    AdjustDialog(term, manager, "adjust-scrollback", parent)
{
    setWindowTitle(TR_TITLE1);

    m_spin = new QSpinBox;
    m_spin->setRange(0, TERM_MAX_CAPORDER);
    m_label = new QLabel;

    QHBoxLayout *layout = new QHBoxLayout;
    layout->addWidget(new QLabel(TR_FIELD1));
    layout->addWidget(m_spin, 1);
    layout->addWidget(new QLabel(TR_FIELD2));
    layout->addWidget(m_label, 1);

    m_mainLayout->addSpacing(m_label->sizeHint().height());
    m_mainLayout->addLayout(layout);
    m_mainLayout->addSpacing(m_label->sizeHint().height());
    m_mainLayout->addWidget(m_allCheck);
    m_mainLayout->addWidget(m_buttonBox);

    connect(m_spin, SIGNAL(valueChanged(int)), this, SLOT(handleValueChanged(int)));
}

void
PowerDialog::handleValueChanged(int power)
{
    m_label->setText(QString::number(1 << power));
}

void
PowerDialog::handleAccept()
{
    g_listener->pushTermCaporder(m_term, m_spin->value());

    if (m_allCheck->isChecked()) {
        for (auto term: m_manager->terms())
            if (term != m_term && term->profileName() == m_profileName)
                g_listener->pushTermCaporder(term, m_spin->value());
    }
    accept();
}

void
PowerDialog::handleReset()
{
    m_spin->setValue(m_term->profile()->scrollbackSize());
}

void
PowerDialog::setValue(unsigned power)
{
    m_spin->setValue(m_value = power);
    handleValueChanged(m_value);
}
