// Copyright © 2018 TermySequence LLC
//
// SPDX-License-Identifier: GPL-2.0-only

#include "common.h"
#include "app/attrbase.h"
#include "app/iconbutton.h"
#include "app/messagebox.h"
#include "base/manager.h"
#include "base/term.h"
#include "colordialog.h"
#include "colorpreview.h"
#include "palettedialog.h"
#include "themecombo.h"
#include "profile.h"
#include "theme.h"

#include <QLabel>
#include <QComboBox>
#include <QCheckBox>
#include <QGridLayout>
#include <QVBoxLayout>

#define TR_BUTTON1 TL("input-button", "Select Color") + A("...")
#define TR_BUTTON2 TL("input-button", "Theme") + A("...")
#define TR_FIELD1 TL("input-field", "Theme") + ':'
#define TR_FIELD2 TL("input-field", "Background") + ':'
#define TR_FIELD3 TL("input-field", "Foreground") + ':'
#define TR_TITLE1 TL("window-title", "Adjust Colors")

ColorDialog::ColorDialog(TermInstance *term, TermManager *manager, QWidget *parent) :
    AdjustDialog(term, manager, "adjust-colors", parent),
    m_palette(term->palette()),
    m_savedPalette(m_palette)
{
    setWindowTitle(TR_TITLE1);

    m_bgPreview = new ColorPreview;
    m_fgPreview = new ColorPreview;
    m_combo = new ThemeCombo(&m_palette);
    QPushButton *bgButton = new IconButton(ICON_CHOOSE_COLOR, TR_BUTTON1);
    QPushButton *fgButton = new IconButton(ICON_CHOOSE_COLOR, TR_BUTTON1);
    QPushButton *palButton = new IconButton(ICON_EDIT_ITEM, TR_BUTTON2);

    QLabel *label;
    QGridLayout *layout = new QGridLayout;
    layout->addWidget(label = new QLabel(TR_FIELD1), 0, 0);
    layout->addWidget(m_combo, 0, 1);
    layout->addWidget(palButton, 0, 2);
    layout->addWidget(new QLabel(TR_FIELD2), 1, 0);
    layout->addWidget(m_bgPreview, 1, 1);
    layout->addWidget(bgButton, 1, 2);
    layout->addWidget(new QLabel(TR_FIELD3), 2, 0);
    layout->addWidget(m_fgPreview, 2, 1);
    layout->addWidget(fgButton, 2, 2);
    layout->setColumnStretch(1, 1);

    m_mainLayout->addSpacing(label->sizeHint().height());
    m_mainLayout->addLayout(layout);
    m_mainLayout->addSpacing(label->sizeHint().height());
    m_mainLayout->addWidget(m_allCheck);
    m_mainLayout->addWidget(m_buttonBox);

    m_bgPreview->setColor(m_palette.bg());
    m_fgPreview->setColor(m_palette.fg());

    connect(bgButton, SIGNAL(clicked()), SLOT(handleBackgroundSelect()));
    connect(fgButton, SIGNAL(clicked()), SLOT(handleForegroundSelect()));
    connect(palButton, SIGNAL(clicked()), SLOT(handlePaletteEdit()));

    connect(m_combo, &ThemeCombo::themeChanged, this, &ColorDialog::handleThemeCombo);
}

void
ColorDialog::handleThemeCombo(const ThemeSettings *theme)
{
    m_term->setPalette(m_palette = theme->content());
    m_bgPreview->setColor(m_palette.bg());
    m_fgPreview->setColor(m_palette.fg());
}

void
ColorDialog::handleBackgroundSelect()
{
    QColor saved = m_palette.bg();

    auto *dialog = colorBox(saved, this);
    connect(dialog, &QColorDialog::currentColorChanged, this, &ColorDialog::handleBackgroundChange);
    connect(dialog, &QDialog::finished, [=](int result) {
        if (result != QDialog::Accepted) {
            handleBackgroundChange(saved);
        }
        m_combo->updateThemes();
    });
    dialog->show();
}

void
ColorDialog::handleBackgroundChange(const QColor &color)
{
    m_bgPreview->setColor(color);
    m_palette.setBg(color.rgb());
    m_term->setPalette(m_palette);
}

void
ColorDialog::handleForegroundSelect()
{
    QColor saved = m_palette.fg();

    auto *dialog = colorBox(saved, this);
    connect(dialog, &QColorDialog::currentColorChanged, this, &ColorDialog::handleForegroundChange);
    connect(dialog, &QDialog::finished, [=](int result) {
        if (result != QDialog::Accepted) {
            handleForegroundChange(saved);
        }
        m_combo->updateThemes();
    });
    dialog->show();
}

void
ColorDialog::handleForegroundChange(const QColor &color)
{
    m_fgPreview->setColor(color);
    m_palette.setFg(color.rgb());
    m_term->setPalette(m_palette);
}

void
ColorDialog::handlePaletteEdit()
{
    auto *dialog = new PaletteDialog(m_palette, m_term->font(), this);
    connect(dialog, &PaletteDialog::termcolorsModified, [=]{
        m_term->setPalette(m_palette = dialog->palette());
        m_bgPreview->setColor(m_palette.bg());
        m_fgPreview->setColor(m_palette.fg());
        m_combo->updateThemes();
    });
    connect(dialog, &PaletteDialog::dircolorsModified, [=]{
        m_term->setPalette(m_palette = dialog->palette());
        m_combo->updateThemes();
    });
    connect(dialog, &QDialog::rejected, [=]{
        m_term->setPalette(m_palette = dialog->saved());
        m_bgPreview->setColor(m_palette.bg());
        m_fgPreview->setColor(m_palette.fg());
        m_combo->updateThemes();
    });

    m_dialog = dialog;
    connect(dialog, &QDialog::finished, [this]{ m_dialog = nullptr; });
    dialog->bringUp();
}

void
ColorDialog::handleAccept()
{
    if (m_allCheck->isChecked()) {
        for (auto term: m_manager->terms())
            if (term != m_term && term->profileName() == m_profileName) {
                term->setPalette(m_palette);
            }
    }
    accept();
}

void
ColorDialog::handleRejected()
{
    if (m_savedPalette != m_palette) {
        m_term->setPalette(m_savedPalette);
    }
}

void
ColorDialog::handleReset()
{
    m_term->setPalette(m_palette = m_term->profile()->content());
    m_bgPreview->setColor(m_palette.bg());
    m_fgPreview->setColor(m_palette.fg());
    m_combo->updateThemes();
}
