// Copyright © 2018 TermySequence LLC
//
// SPDX-License-Identifier: GPL-2.0-only

#pragma once

#include "settingwidget.h"

QT_BEGIN_NAMESPACE
class QSpinBox;
QT_END_NAMESPACE
class FontPreview;

class FontSelect final: public SettingWidget
{
    Q_OBJECT

private:
    QSpinBox *m_spin;
    FontPreview *m_preview;

    QFont m_font;

    void handleFontResult(const QFont &font);

private slots:
    void handleFontSize(int size);
    void handleFontSelect();
    void handleMonospaceSelect();

protected:
    void handleSettingChanged(const QVariant &value);

public:
    FontSelect(const SettingDef *def, SettingsBase *settings, bool monospace);
};


class FontSelectFactory final: public SettingWidgetFactory
{
private:
    bool m_monospace;

public:
    FontSelectFactory(bool monospace);

    QWidget* createWidget(const SettingDef *def, SettingsBase *settings) const;
};
