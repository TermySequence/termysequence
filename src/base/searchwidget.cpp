// Copyright © 2018 TermySequence LLC
//
// SPDX-License-Identifier: GPL-2.0-only

#include "common.h"
#include "app/iconbutton.h"
#include "searchwidget.h"
#include "manager.h"
#include "mainwindow.h"
#include "term.h"
#include "scrollport.h"
#include "settings/state.h"

#include <QLabel>
#include <QComboBox>
#include <QCheckBox>
#include <QLineEdit>
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QRegularExpression>

#define STATE_VERSION 1

#define TR_BUTTON1 TL("input-button", "Search Up")
#define TR_BUTTON2 TL("input-button", "Search Down")
#define TR_BUTTON3 TL("input-button", "Reset Search")
#define TR_CHECK1 TL("input-checkbox", "Match Case")
#define TR_SEARCH1 TL("search-type", "Single Line Plain Text")
#define TR_SEARCH2 TL("search-type", "Single Line ECMAScript Regex")
#define TR_TEXT1 TL("window-text", "No terminal is active")
#define TR_TEXT2 TL("window-text", "Press Return to return to terminal")

SearchWidget::SearchWidget(TermManager *manager) :
    SearchableWidget(manager)
{
    m_combo = new QComboBox;
    m_combo->addItem(TR_SEARCH1);
    m_combo->addItem(TR_SEARCH2);
    m_check = new QCheckBox(TR_CHECK1);

    m_status = new QLabel;

    QHBoxLayout *searchLayout = new QHBoxLayout;
    searchLayout->addWidget(m_combo);
    searchLayout->addWidget(m_line, 1);
    searchLayout->addWidget(m_check);

    auto prevButton = new IconButton(ICON_SEARCH_UP, TR_BUTTON1);
    auto nextButton = new IconButton(ICON_SEARCH_DOWN, TR_BUTTON2);
    auto resetButton = new IconButton(ICON_RESET_SEARCH, TR_BUTTON3);
    QHBoxLayout *buttonLayout = new QHBoxLayout;
    buttonLayout->addWidget(prevButton);
    buttonLayout->addWidget(nextButton);
    buttonLayout->addWidget(m_status);
    buttonLayout->addStretch(1);
    buttonLayout->addWidget(resetButton);

    QVBoxLayout *mainLayout = new QVBoxLayout;
    // mainLayout->setContentsMargins(g_mtmargins);
    // mainLayout->setSpacing(0);
    mainLayout->addLayout(searchLayout);
    mainLayout->addLayout(buttonLayout);
    setLayout(mainLayout);

    connect(manager,
            SIGNAL(termActivated(TermInstance*,TermScrollport*)),
            SLOT(handleTermActivated(TermInstance*,TermScrollport*)));

    connectWidgets();
    connect(m_line, SIGNAL(returnPressed()), SLOT(handleReturnPressed()));

    connect(prevButton, SIGNAL(clicked()), manager, SLOT(actionSearchUp()));
    connect(nextButton, SIGNAL(clicked()), manager, SLOT(actionSearchDown()));
    connect(resetButton, SIGNAL(clicked()), SLOT(handleSearchReset()));
    handleTermActivated(nullptr, nullptr);
}

void
SearchWidget::connectWidgets()
{
    m_mocCombo = connect(m_combo, SIGNAL(currentIndexChanged(int)), m_line, SLOT(clear()));
    m_mocLine = connect(m_line, SIGNAL(textChanged(const QString&)), SLOT(handleSearchText()));
    m_mocCheck = connect(m_check, SIGNAL(toggled(bool)), SLOT(handleSearchText()));
}

void
SearchWidget::disconnectWidgets()
{
    disconnect(m_mocCombo);
    disconnect(m_mocLine);
    disconnect(m_mocCheck);
}

void
SearchWidget::handleSearchChanged()
{
    if (!m_maskTerm) {
        const TermSearch &search = m_term->search();

        disconnectWidgets();

        if (search.active) {
            m_combo->setCurrentIndex(search.type);
            m_check->setChecked(search.matchCase);
        }
        m_line->setText(search.text);
        m_status->setText(m_scrollport->searchStatus());

        connectWidgets();
    }
}

void
SearchWidget::handleTermActivated(TermInstance *term, TermScrollport *scrollport)
{
    if (m_term) {
        disconnect(m_mocTerm);
        disconnect(m_mocStatus);
    }

    if ((m_term = term)) {
        m_scrollport = scrollport;
        m_mocTerm = connect(term, SIGNAL(searchChanged(bool)),
                            SLOT(handleSearchChanged()));
        m_mocStatus = connect(scrollport, SIGNAL(searchStatusChanged(QString)),
                              m_status, SLOT(setText(const QString&)));

        m_combo->setEnabled(true);
        m_line->setEnabled(true);
        m_check->setEnabled(true);

        handleSearchChanged();
    }
    else {
        m_combo->setCurrentIndex(0);
        m_combo->setEnabled(false);
        m_line->clear();
        m_line->setEnabled(false);
        m_check->setEnabled(false);
        m_status->setText(TR_TEXT1);
    }
}

void
SearchWidget::handleSearchText()
{
    if (m_term) {
        TermSearch search;
        QString tmp;

        try {
            search.type = (Tsqt::SearchType)m_combo->currentIndex();
            search.text = m_line->text();
            search.matchCase = m_check->isChecked();

            auto flags = std::regex::nosubs|std::regex::optimize;
            if (!search.matchCase)
                flags |= std::regex::icase;

            switch(search.type) {
            case Tsqt::SingleLineEcmaRegex:
                search.regex.assign(search.text.toStdString(), flags);
                break;
            default:
                tmp = QRegularExpression::escape(search.text);
                search.regex.assign(tmp.toStdString(), flags);
                break;
            }
            search.active = !search.text.isEmpty();
        }
        catch (const std::exception &) {
            // do nothing
        }

        m_maskTerm = true;
        m_term->setSearch(search);
        m_maskTerm = false;
    }
}

void
SearchWidget::handleReturnPressed()
{
    if (m_line->text().isEmpty())
        m_window->unpopDockWidget(m_window->searchDock(), true);
}

void
SearchWidget::handleEscapePressed()
{
    m_window->unpopDockWidget(m_window->searchDock(), true);
}

void
SearchWidget::handleFocusIn()
{
    m_status->setText(TR_TEXT2);
}

void
SearchWidget::handleFocusOut()
{
    m_status->setText(m_scrollport->searchStatus());
}

void
SearchWidget::handleSearchReset()
{
    m_line->clear();
    m_line->setFocus(Qt::OtherFocusReason);
}

void
SearchWidget::toolAction(int index)
{
    switch (index) {
    case 0:
        m_manager->actionSearchUp();
        break;
    case 1:
        m_manager->actionSearchDown();
        break;
    }
}

void
SearchWidget::restoreState(int index)
{
    // Restore our own settings
    auto bytes = g_state->fetchVersioned(SearchSettingsKey, STATE_VERSION, index, 2);
    if (!bytes.isEmpty()) {
        // Search type and match case
        int searchType = bytes[0];
        if (searchType >= 0 && searchType < m_combo->count())
            m_combo->setCurrentIndex(searchType);

        m_check->setChecked(bytes[1]);
    }
}

void
SearchWidget::saveState(int index)
{
    QByteArray bytes;

    // Save our own settings
    bytes.append((char)m_combo->currentIndex());
    bytes.append((char)m_check->isChecked());

    g_state->storeVersioned(SearchSettingsKey, STATE_VERSION, bytes, index);
}
