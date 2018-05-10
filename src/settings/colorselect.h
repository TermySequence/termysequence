// Copyright © 2018 TermySequence LLC
//
// SPDX-License-Identifier: GPL-2.0-only

#pragma once

#include "settingwidget.h"
#include "termcolors.h"

QT_BEGIN_NAMESPACE
class QLineEdit;
QT_END_NAMESPACE
class ColorPreview;

class ColorSelect final: public SettingWidget
{
    Q_OBJECT

private:
    QLineEdit *m_text;
    ColorPreview *m_preview;

    int m_paletteIdx;
    Termcolors m_palette;

private slots:
    void handleColorText();
    void handleColorSelect();
    void handleColorChange(const QColor &color);

protected:
    void handleSettingChanged(const QVariant &value);

public:
    ColorSelect(const SettingDef *def, SettingsBase *settings, int paletteIdx);
};


class ColorSelectFactory final: public SettingWidgetFactory
{
private:
    int m_paletteIdx;

public:
    ColorSelectFactory(int paletteIdx);

    QWidget* createWidget(const SettingDef *def, SettingsBase *settings) const;
};
