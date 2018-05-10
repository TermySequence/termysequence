// Copyright © 2018 TermySequence LLC
//
// SPDX-License-Identifier: GPL-2.0-only

#include "common.h"
#include "app/iconbutton.h"
#include "app/messagebox.h"
#include "base/manager.h"
#include "base/term.h"
#include "fontdialog.h"
#include "fontpreview.h"
#include "profile.h"

#include <QSpinBox>
#include <QLabel>
#include <QCheckBox>
#include <QHBoxLayout>
#include <QVBoxLayout>

#define TR_BUTTON1 TL("input-button", "Select Font") + A("...")
#define TR_DIM1 ' ' + TL("settings-dimension", "pt")
#define TR_FIELD1 TL("input-field", "Size") + ':'
#define TR_TITLE1 TL("window-title", "Adjust Font")
#define TR_TITLE2 TL("window-title", "Select Monospace Font")

FontDialog::FontDialog(TermInstance *term, TermManager *manager, QWidget *parent) :
    AdjustDialog(term, manager, "adjust-font", parent)
{
    setWindowTitle(TR_TITLE1);

    QPushButton *button = new IconButton(ICON_CHOOSE_FONT, TR_BUTTON1);
    m_spin = new QSpinBox;
    m_spin->setRange(1, 256);
    m_spin->setSuffix(TR_DIM1);
    m_preview = new FontPreview;

    QHBoxLayout *layout = new QHBoxLayout;
    QLabel *label = new QLabel(TR_FIELD1);
    layout->addWidget(label);
    layout->addWidget(m_spin);
    layout->addStretch(1);
    layout->addWidget(button);

    m_mainLayout->addSpacing(label->sizeHint().height());
    m_mainLayout->addLayout(layout);
    m_mainLayout->addWidget(m_preview);
    m_mainLayout->addSpacing(label->sizeHint().height());
    m_mainLayout->addWidget(m_allCheck);
    m_mainLayout->addWidget(m_buttonBox);

    m_preview->setFont(m_savedFont = m_font = term->realFont());
    m_spin->setValue(m_font.pointSize());

    connect(m_spin, SIGNAL(valueChanged(int)), SLOT(handleFontSize(int)));
    connect(button, SIGNAL(clicked()), SLOT(handleFontSelect()));
}

void
FontDialog::handleFontSize(int size)
{
    if (size != m_font.pointSize()) {
        m_font.setPointSize(size);
        m_preview->setFont(m_font);
        m_term->setFont(m_font);
    }
}

void
FontDialog::handleFontResult(const QFont &font)
{
    m_preview->setFont(m_font = font);
    m_spin->setValue(font.pointSize());
    m_term->setFont(font);
}

void
FontDialog::handleFontSelect()
{
    const QFont savedDialogFont = m_font;

    auto *box = fontBox(TR_TITLE2, m_font, true, this);
    connect(box, &QFontDialog::currentFontChanged, this, &FontDialog::handleFontResult);
    connect(box, &QFontDialog::fontSelected, this, &FontDialog::handleFontResult);
    connect(box, &QDialog::rejected, [=]{ handleFontResult(savedDialogFont); });
    box->show();
}

void
FontDialog::handleAccept()
{
    if (m_allCheck->isChecked()) {
        for (auto term: m_manager->terms())
            if (term != m_term && term->profileName() == m_profileName)
                term->setFont(m_font);
    }
    accept();
}

void
FontDialog::handleRejected()
{
    if (m_font != m_savedFont) {
        m_term->setFont(m_savedFont);
    }
}

void
FontDialog::handleReset()
{
    m_font.fromString(m_term->profile()->font());

    m_preview->setFont(m_font);
    m_spin->setValue(m_font.pointSize());
}
