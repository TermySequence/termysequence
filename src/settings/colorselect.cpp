// Copyright © 2018 TermySequence LLC
//
// SPDX-License-Identifier: GPL-2.0-only

#include "common.h"
#include "app/attrbase.h"
#include "app/iconbutton.h"
#include "app/messagebox.h"
#include "colorselect.h"
#include "base.h"
#include "colorpreview.h"

#include <QLineEdit>
#include <QLabel>
#include <QHBoxLayout>
#include <QRegularExpression>

#define TR_BUTTON1 TL("input-button", "Color") + A("...")
#define TR_FIELD1 TL("input-field", "Hex") + ':'

ColorSelect::ColorSelect(const SettingDef *def, SettingsBase *settings, int paletteIdx) :
    SettingWidget(def, settings),
    m_paletteIdx(paletteIdx)
{
    QPushButton *button = new IconButton(ICON_CHOOSE_COLOR, TR_BUTTON1);
    m_text = new QLineEdit;
    m_text->setMaxLength(64);
    m_preview = new ColorPreview;

    QHBoxLayout *layout = new QHBoxLayout;
    layout->setContentsMargins(g_mtmargins);
    layout->addWidget(m_preview, 1);
    layout->addWidget(new QLabel(TR_FIELD1));
    layout->addWidget(m_text, 1);
    layout->addWidget(button);
    setLayout(layout);

    handleSettingChanged(m_value);

    connect(m_text, SIGNAL(editingFinished()), SLOT(handleColorText()));
    connect(button, SIGNAL(clicked()), SLOT(handleColorSelect()));
}

void
ColorSelect::handleColorText()
{
    QString trimmed = m_text->text().trimmed();

    // Try to put a hash mark on the front of hex chars
    QRegularExpression hexRe(L("\\A[0-9A-Fa-f]+\\z"));
    if (hexRe.match(trimmed).hasMatch())
        trimmed.prepend('#');

    QColor tmp(trimmed);
    if (tmp.isValid()) {
        m_palette[m_paletteIdx] = tmp.rgb() & PALETTE_COLOR;
        setProperty(m_palette.tStr());
    }
}

void
ColorSelect::handleColorSelect()
{
    QColor saved = m_palette.at(m_paletteIdx);

    auto *dialog = colorBox(saved, this);
    connect(dialog, &QColorDialog::currentColorChanged, this, &ColorSelect::handleColorChange);
    connect(dialog, &QDialog::rejected, [=]{ handleColorChange(saved); });
    dialog->show();
}

void
ColorSelect::handleColorChange(const QColor &color)
{
    m_palette[m_paletteIdx] = color.rgb() & PALETTE_COLOR;
    setProperty(m_palette.tStr());
}

void
ColorSelect::handleSettingChanged(const QVariant &value)
{
    m_palette.parse(value.toString());
    QColor color(m_palette.at(m_paletteIdx));

    m_text->setText(color.name(QColor::HexRgb));
    m_preview->setColor(color);
}


ColorSelectFactory::ColorSelectFactory(int paletteIdx) :
    m_paletteIdx(paletteIdx)
{
}

QWidget *
ColorSelectFactory::createWidget(const SettingDef *def, SettingsBase *settings) const
{
    // Increment the SettingDef pointer forward until its property is non-null
    // This setting corresponds to the palette property that we will use
    while (def->property == nullptr)
        ++def;

    return new ColorSelect(def, settings, m_paletteIdx);
}
