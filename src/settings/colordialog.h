// Copyright © 2018 TermySequence LLC
//
// SPDX-License-Identifier: GPL-2.0-only

#pragma once

#include "adjustdialog.h"
#include "palette.h"

class ColorPreview;
class PaletteDialog;
class ThemeCombo;
class ThemeSettings;

class ColorDialog final: public AdjustDialog
{
    Q_OBJECT

private:
    ThemeCombo *m_combo;
    ColorPreview *m_bgPreview;
    ColorPreview *m_fgPreview;

    TermPalette m_palette, m_savedPalette;

private slots:
    void handleThemeCombo(const ThemeSettings *theme);
    void handleBackgroundSelect();
    void handleBackgroundChange(const QColor &color);
    void handleForegroundSelect();
    void handleForegroundChange(const QColor &color);
    void handlePaletteEdit();

    void handleAccept();
    void handleRejected();
    void handleReset();

public:
    ColorDialog(TermInstance *term, TermManager *manager, QWidget *parent);
};
