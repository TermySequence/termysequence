// Copyright © 2018 TermySequence LLC
//
// SPDX-License-Identifier: GPL-2.0-only

#include "common.h"
#include "app/attrbase.h"
#include "app/iconbutton.h"
#include "palettewidget.h"
#include "palettedialog.h"
#include "themecombo.h"
#include "theme.h"

#include <QHBoxLayout>
#include <QFontDatabase>

#define TR_BUTTON1 TL("input-button", "Theme") + A("...")

PaletteWidget::PaletteWidget(const SettingDef *def, SettingsBase *settings) :
    SettingWidget(def, settings)
{
    m_scratch = new TermPalette(m_value.toString(), getOther("dircolors").toString());
    m_combo = new ThemeCombo(m_scratch);
    m_combo->installEventFilter(this);

    QPushButton *button = new IconButton(ICON_EDIT_ITEM, TR_BUTTON1);

    QHBoxLayout *layout = new QHBoxLayout;
    layout->setContentsMargins(g_mtmargins);
    layout->addWidget(m_combo, 1);
    layout->addWidget(button);
    setLayout(layout);

    connect(m_combo, &ThemeCombo::themeChanged, this, &PaletteWidget::handleCombo);
    connect(button, SIGNAL(clicked()), SLOT(handleClicked()));
    connect(settings, SIGNAL(settingChanged(const char*,QVariant)),
            SLOT(handleOtherChanged(const char*,QVariant)));
}

PaletteWidget::~PaletteWidget()
{
    delete m_scratch;
}

void
PaletteWidget::handleCombo(const ThemeSettings *theme)
{
    setProperty(theme->content().tStr());
    setOther("dircolors", theme->content().dStr());
}

void
PaletteWidget::handleClicked()
{
    QFont font;
    if (!font.fromString(getOther("font").toString()))
        font = QFontDatabase::systemFont(QFontDatabase::FixedFont);

    auto *dialog = new PaletteDialog(*m_scratch, font, this);
    connect(dialog, &PaletteDialog::termcolorsModified, [=]{
        setProperty(dialog->palette().tStr());
    });
    connect(dialog, &PaletteDialog::dircolorsModified, [=]{
        setOther("dircolors", dialog->palette().dStr());
    });
    connect(dialog, &QDialog::rejected, [=]{
        setProperty(dialog->saved().tStr());
        setOther("dircolors", dialog->saved().dStr());
    });
    dialog->bringUp();
}

void
PaletteWidget::handleSettingChanged(const QVariant &value)
{
    m_scratch->Termcolors::operator=(value.toString());
    m_combo->updateThemes();
    // Note: not updating dialog in response to external change
}

void
PaletteWidget::handleOtherChanged(const char *property, const QVariant &value)
{
    if (strcmp(property, "dircolors") == 0) {
        m_scratch->Dircolors::operator=(value.toString());
        m_combo->updateThemes();
    }
}

QWidget *
PaletteWidgetFactory::createWidget(const SettingDef *def, SettingsBase *settings) const
{
    return new PaletteWidget(def, settings);
}
