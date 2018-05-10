// Copyright © 2018 TermySequence LLC
//
// SPDX-License-Identifier: GPL-2.0-only

#pragma once

#include "base.h"
#include "palette.h"

struct ThemeDef;

class ThemeSettings final: public SettingsBase
{
    Q_OBJECT
    // Hidden
    Q_PROPERTY(QString group READ group WRITE setGroup)
    Q_PROPERTY(QString palette READ palette WRITE setPalette)
    Q_PROPERTY(QString dircolors READ dircolors WRITE setDircolors)
    Q_PROPERTY(bool lesser READ lesser WRITE setLesser)

private:
    // Non_properties
    QString m_name;
    TermPalette m_content;
    int m_order;

    // Properties
    REFPROP(QString, group, setGroup)
    REFPROP(QString, palette, setPalette)
    REFPROP(QString, dircolors, setDircolors)
    VALPROP(bool, lesser, setLesser)

public:
    ThemeSettings(const QString &name, const QString &path);
    ThemeSettings(const ThemeDef *def, int order);

    // Non-properties
    inline const QString& name() const { return m_name; }
    inline const TermPalette& content() const { return m_content; }

    inline int order() const { return m_order; }
    inline bool builtin() const { return m_order != -1; }

    void setName(const QString &name); // Non-file-backed only
};
