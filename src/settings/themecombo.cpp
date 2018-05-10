// Copyright © 2018 TermySequence LLC
//
// SPDX-License-Identifier: GPL-2.0-only

#include "common.h"
#include "app/attrbase.h"
#include "themecombo.h"
#include "settings.h"
#include "palette.h"
#include "theme.h"

#define TR_NAME1 TL("settings-name", "Custom Color Theme")

ThemeCombo::ThemeCombo(const TermPalette *palette) :
    m_palette(palette)
{
    connect(g_settings, &TermSettings::themesChanged, this, &ThemeCombo::updateThemes);
    updateThemes();
}

void
ThemeCombo::updateThemes()
{
    disconnect(m_mocIndex);
    clear();

    int i = 0, themeIdx = -1;

    for (auto theme: g_settings->themes()) {
        if (*m_palette == theme->content()) {
            themeIdx = i;
        } else if (theme->lesser()) {
            continue;
        }
        addItem(theme->name());
        setItemData(i, QColor(theme->content().fg()), Qt::ForegroundRole);
        setItemData(i, QColor(theme->content().bg()), Qt::BackgroundRole);
        ++i;
    }

    if ((m_haveCustom = (themeIdx == -1))) {
        insertItem(0, L("<%1>").arg(TR_NAME1));
        setItemData(0, QColor(m_palette->fg()), Qt::ForegroundRole);
        setItemData(0, QColor(m_palette->bg()), Qt::BackgroundRole);
        insertSeparator(1);
        setCurrentIndex(0);
    } else {
        setCurrentIndex(themeIdx);
    }

    m_mocIndex = connect(this, SIGNAL(currentIndexChanged(int)),
                         SLOT(handleIndexChanged(int)));
}

void
ThemeCombo::handleIndexChanged(int index)
{
    auto *theme = g_settings->theme(currentText());
    if (theme) {
        emit themeChanged(theme);
        if (index > 1 && m_haveCustom) {
            disconnect(m_mocIndex);
            removeItem(1);
            removeItem(0);
            setCurrentIndex(index - 2);
            m_haveCustom = false;
            m_mocIndex = connect(this, SIGNAL(currentIndexChanged(int)),
                                 SLOT(handleIndexChanged(int)));
        }
    }
}
