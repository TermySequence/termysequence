// Copyright © 2018 TermySequence LLC
//
// SPDX-License-Identifier: GPL-2.0-only

#include "common.h"
#include "app/attr.h"
#include "app/iconbutton.h"
#include "base/term.h"
#include "base/mark.h"
#include "base/listener.h"
#include "notedialog.h"

#include <QLabel>
#include <QLineEdit>
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QEvent>

#define TR_BUTTON1 TL("input-button", "Reset Note Index")
#define TR_FIELD1 TL("input-field", "Note mark character (optional)") + ':'
#define TR_FIELD2 TL("input-field", "Note text (optional)") + ':'
#define TR_TITLE1 TL("window-title", "New Annotation")

NoteDialog::NoteDialog(TermInstance *term, const RegionBase *region, QWidget *parent) :
    QDialog(parent),
    m_term(term),
    m_region(*region)
{
    setWindowTitle(TR_TITLE1);
    setAttribute(Qt::WA_DeleteOnClose, true);
    setWindowModality(Qt::WindowModal);
    setSizeGripEnabled(true);

    int noteNum = term->noteNum();
    QString noteChar = TermMark::base36(noteNum, '+');

    m_char = new QLineEdit(noteChar);
    m_char->setMaxLength(1);
    m_note = new QLineEdit;

    QDialogButtonBox *buttonBox = new QDialogButtonBox(
        QDialogButtonBox::Ok|QDialogButtonBox::Cancel);

    QPushButton *resetButton = new IconButton(ICON_RESET, TR_BUTTON1);

    QHBoxLayout *charLayout = new QHBoxLayout;
    charLayout->addWidget(m_char, 1);
    charLayout->addWidget(resetButton);

    QVBoxLayout *layout = new QVBoxLayout;
    layout->addWidget(new QLabel(TR_FIELD1));
    layout->addLayout(charLayout);
    layout->addWidget(new QLabel(TR_FIELD2));
    layout->addWidget(m_note);
    layout->addWidget(buttonBox);
    setLayout(layout);

    connect(term, SIGNAL(destroyed()), SLOT(deleteLater()));
    connect(buttonBox, SIGNAL(accepted()), SLOT(handleAccept()));
    connect(buttonBox, SIGNAL(rejected()), SLOT(reject()));
    connect(resetButton, SIGNAL(clicked()), SLOT(handleReset()));
}

bool
NoteDialog::event(QEvent *event)
{
    switch (event->type()) {
    case QEvent::WindowActivate:
        m_note->setFocus(Qt::ActiveWindowFocusReason);
        break;
    default:
        break;
    }

    return QDialog::event(event);
}

void
NoteDialog::handleReset()
{
    m_term->resetNoteNum();
    m_char->setText(QString::number(m_term->noteNum(), 36));
}

void
NoteDialog::handleAccept()
{
    QString noteChar = m_char->text();
    QString noteText = m_note->text();

    if (!noteChar.isEmpty())
        m_region.attributes[g_attr_REGION_NOTECHAR] = noteChar.at(0);
    if (!noteText.isEmpty())
        m_region.attributes[g_attr_REGION_NOTETEXT] = noteText;

    m_term->incNoteNum();

    g_listener->pushRegionCreate(m_term, &m_region);
    accept();
}
