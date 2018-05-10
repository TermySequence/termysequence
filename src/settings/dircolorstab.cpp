// Copyright © 2018 TermySequence LLC
//
// SPDX-License-Identifier: GPL-2.0-only

#include "common.h"
#include "app/attrbase.h"
#include "app/iconbutton.h"
#include "dircolorstab.h"
#include "dircolorsmodel.h"

#include <QCheckBox>
#include <QPlainTextEdit>
#include <QLabel>
#include <QVBoxLayout>
#include <QHBoxLayout>

#define TR_BUTTON1 TL("input-button", "Update")
#define TR_CHECK1 TL("input-checkbox", "Inherit default dataset")
#define TR_ERROR1 TL("error", "Failed to parse the text")
#define TR_TEXT1 TL("window-text", "Click update to reparse the text")
#define TR_TEXT2 TL("window-text", "Successfully parsed the text")
#define TR_TITLE1 TL("window-title", "Adjust Dircolors")

DircolorsTab::DircolorsTab(TermPalette &palette, const QFont &font) :
    m_palette(palette)
{
    m_view = new DircolorsView(palette, font);

    m_text = new QPlainTextEdit;
    m_text->setWordWrapMode(QTextOption::WrapAnywhere);
    m_inherit = new QCheckBox(TR_CHECK1);
    m_status = new QLabel;
    m_updateButton = new IconButton(ICON_RELOAD, TR_BUTTON1);

    QHBoxLayout *statusLayout = new QHBoxLayout;
    statusLayout->addWidget(m_updateButton);
    statusLayout->addWidget(m_status, 1);

    QVBoxLayout *layout = new QVBoxLayout;
    layout->addWidget(m_view, 1);
    layout->addLayout(statusLayout);
    layout->addWidget(m_inherit);
    layout->addWidget(m_text, 1);
    setLayout(layout);

    m_text->setPlainText(palette.dStr());
    reload();

    connect(m_view, &DircolorsView::selectedSubstring, this, &DircolorsTab::handleSelect);
    connect(m_view, &DircolorsView::appendedSubstring, this, &DircolorsTab::handleAppend);
    connect(m_updateButton, SIGNAL(clicked()), SLOT(handleUpdate()));
    connect(m_text, SIGNAL(textChanged()), SLOT(handleTextChanged()));
    connect(m_inherit, SIGNAL(toggled(bool)), SLOT(handleInherit(bool)));
}

void
DircolorsTab::reload()
{
    const auto &text = m_palette.dStr();
    m_text->setPlainText(text);
    m_inherit->setChecked(text.isEmpty() || text.startsWith('+'));
    m_status->clear();
    m_view->reloadData();
}

void
DircolorsTab::handleUpdate()
{
    bool parseOk;
    Dircolors scratch(m_text->toPlainText(), &parseOk);

    if (parseOk) {
        m_palette.Dircolors::operator=(scratch);
        reload();
        m_status->setText(TR_TEXT2);
        emit modified();
    } else {
        m_status->setText(TR_ERROR1);
    }
}

void
DircolorsTab::handleTextChanged()
{
    QString text = m_text->toPlainText();
    m_inherit->setChecked(text.isEmpty() || text.startsWith('+'));
    m_status->setText(TR_TEXT1);
}

void
DircolorsTab::handleInherit(bool inherit)
{
    QString text = m_text->toPlainText();

    if (!inherit && text.isEmpty()) {
        m_status->setText(TR_TEXT1);
    }
    else if (inherit && !text.isEmpty() && !text.startsWith('+')) {
        text.prepend('+');
        m_text->setPlainText(text);
    }
    else if (!inherit && text.startsWith('+')) {
        text.remove(0, 1);
        m_text->setPlainText(text);
    }
}

void
DircolorsTab::handleSelect(int start, int end)
{
    QTextCursor tc(m_text->document());
    tc.movePosition(QTextCursor::NextCharacter, QTextCursor::MoveAnchor, start);
    tc.movePosition(QTextCursor::NextCharacter, QTextCursor::KeepAnchor, end - start);
    m_text->setTextCursor(tc);
}

void
DircolorsTab::handleAppend(const QString &str)
{
    QString text = m_text->toPlainText();
    if (text.isEmpty())
        text = '+';

    int start = text.size();
    text += str;
    m_text->setPlainText(text);
    handleSelect(start, text.size());
}
