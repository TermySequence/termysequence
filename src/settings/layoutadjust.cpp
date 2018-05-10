// Copyright © 2018 TermySequence LLC
//
// SPDX-License-Identifier: GPL-2.0-only

#include "common.h"
#include "base/manager.h"
#include "base/term.h"
#include "layoutadjust.h"
#include "layouttabs.h"
#include "profile.h"

#include <QCheckBox>
#include <QDialogButtonBox>
#include <QVBoxLayout>

#define TR_TITLE1 TL("window-title", "Adjust Terminal Layout")

LayoutAdjust::LayoutAdjust(TermInstance *term, TermManager *manager, QWidget *parent) :
    AdjustDialog(term, manager, "adjust-layout", parent),
    m_layout(term->layout(), term->fills()),
    m_saved(m_layout)
{
    setWindowTitle(TR_TITLE1);

    m_tabs = new LayoutTabs(m_layout, term->palette(), term->font());

    m_mainLayout->addWidget(m_tabs);
    m_mainLayout->addWidget(m_allCheck);
    m_mainLayout->addWidget(m_buttonBox);

    connect(m_tabs, &LayoutTabs::modified, this, &LayoutAdjust::handleLayout);
}

static inline void
setLayoutOnTerm(TermInstance *term, const TermLayout &layout)
{
    term->setLayout(layout.layoutStr());
    term->setFills(layout.fillsStr());
}

void
LayoutAdjust::handleLayout()
{
    setLayoutOnTerm(m_term, m_layout);
}

void
LayoutAdjust::handleAccept()
{
    if (m_allCheck->isChecked()) {
        for (auto term: m_manager->terms())
            if (term != m_term && term->profileName() == m_profileName)
                setLayoutOnTerm(term, m_layout);
    }
    accept();
}

void
LayoutAdjust::handleRejected()
{
    setLayoutOnTerm(m_term, m_saved);
}

void
LayoutAdjust::handleReset()
{
    const auto *profile = m_term->profile();
    m_layout.parseLayout(profile->layout());
    m_layout.parseFills(profile->fills());
    setLayoutOnTerm(m_term, m_layout);
    m_tabs->reloadData();
}
