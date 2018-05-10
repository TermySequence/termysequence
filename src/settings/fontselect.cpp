// Copyright © 2018 TermySequence LLC
//
// SPDX-License-Identifier: GPL-2.0-only

#include "common.h"
#include "app/attrbase.h"
#include "app/iconbutton.h"
#include "app/messagebox.h"
#include "fontselect.h"
#include "fontpreview.h"

#include <QSpinBox>
#include <QLabel>
#include <QHBoxLayout>
#include <QVBoxLayout>

#define TR_BUTTON1 TL("input-button", "Font") + A("...")
#define TR_FIELD1 TL("input-field", "Size") + ':'
#define TR_TITLE1 TL("window-title", "Select Font")
#define TR_TITLE2 TL("window-title", "Select Monospace Font")

FontSelect::FontSelect(const SettingDef *def, SettingsBase *settings, bool monospace) :
    SettingWidget(def, settings)
{
    QPushButton *button = new IconButton(ICON_CHOOSE_FONT, TR_BUTTON1);
    m_spin = new QSpinBox;
    m_spin->installEventFilter(this);
    m_spin->setRange(1, 256);
    m_preview = new FontPreview;

    QHBoxLayout *topLayout = new QHBoxLayout;
    topLayout->setContentsMargins(g_mtmargins);
    topLayout->addWidget(new QLabel(TR_FIELD1));
    topLayout->addWidget(m_spin);
    topLayout->addWidget(button);
    topLayout->addStretch(1);

    QVBoxLayout *layout = new QVBoxLayout;
    layout->setContentsMargins(g_mtmargins);
    layout->addLayout(topLayout);
    layout->addWidget(m_preview);
    setLayout(layout);

    handleSettingChanged(m_value);

    connect(m_spin, SIGNAL(valueChanged(int)), SLOT(handleFontSize(int)));

    if (monospace)
        connect(button, SIGNAL(clicked()), SLOT(handleMonospaceSelect()));
    else
        connect(button, SIGNAL(clicked()), SLOT(handleFontSelect()));
}

void
FontSelect::handleFontSize(int size)
{
    m_font.setPointSize(size);
    setProperty(m_font.toString());
}

void
FontSelect::handleFontResult(const QFont &font)
{
    setProperty(font.toString());
}

void
FontSelect::handleFontSelect()
{
    const QFont savedDialogFont = m_font;

    auto *box = fontBox(TR_TITLE1, m_font, false, this);
    connect(box, &QFontDialog::currentFontChanged, this, &FontSelect::handleFontResult);
    connect(box, &QFontDialog::fontSelected, this, &FontSelect::handleFontResult);
    connect(box, &QDialog::rejected, [=]{ handleFontResult(savedDialogFont); });
    box->show();
}

void
FontSelect::handleMonospaceSelect()
{
    QFont savedDialogFont = m_font;

    auto *box = fontBox(TR_TITLE2, m_font, true, this);
    connect(box, &QFontDialog::currentFontChanged, this, &FontSelect::handleFontResult);
    connect(box, &QFontDialog::fontSelected, this, &FontSelect::handleFontResult);
    connect(box, &QDialog::rejected, [=]{ handleFontResult(savedDialogFont); });
    box->show();
}

void
FontSelect::handleSettingChanged(const QVariant &value)
{
    m_font.fromString(value.toString());
    m_spin->setValue(m_font.pointSize());
    m_preview->setFont(m_font);
}


FontSelectFactory::FontSelectFactory(bool monospace) :
    m_monospace(monospace)
{
}

QWidget *
FontSelectFactory::createWidget(const SettingDef *def, SettingsBase *settings) const
{
    return new FontSelect(def, settings, m_monospace);
}
