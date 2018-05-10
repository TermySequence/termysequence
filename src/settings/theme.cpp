// Copyright © 2018 TermySequence LLC
//
// SPDX-License-Identifier: GPL-2.0-only

#include "common.h"
#include "app/deftheme.h"
#include "theme.h"

#include <QCoreApplication>

static const SettingDef s_themeDefs[] = {
    { "Theme/Group", "group", QVariant::String,
      NULL, NULL, NULL
    },
    { "Theme/Palette", "palette", QVariant::String,
      NULL, NULL, NULL
    },
    { "Theme/Dircolors", "dircolors", QVariant::String,
      NULL, NULL, NULL
    },
    { "Theme/LowPriority", "lesser", QVariant::Bool,
      NULL, NULL, NULL
    },
    { NULL }
};

static SettingsBase::SettingsDef s_themeDef = {
    SettingsBase::Global, s_themeDefs
};

ThemeSettings::ThemeSettings(const QString &name, const QString &path) :
    SettingsBase(s_themeDef, path),
    m_order(-1),
    m_lesser(false)
{
    m_name = name;
}

ThemeSettings::ThemeSettings(const ThemeDef *def, int order) :
    SettingsBase(s_themeDef),
    m_content(def->palette, def->dircolors),
    m_order(order),
    m_palette(m_content.tStr())
{
    m_name = QCoreApplication::translate("settings-name", def->name);
    m_group = QCoreApplication::translate("settings-name", def->group);
    // initDefaults
    m_lesser = false;
}

/*
 * Non-properties
 */
void
ThemeSettings::setName(const QString &name)
{
    m_name = name;
}

/*
 * Properties
 */
void
ThemeSettings::setGroup(const QString &group)
{
    m_group = group;
}

void
ThemeSettings::setPalette(const QString &palette)
{
    if (m_palette != palette) {
        m_palette = palette;
        m_content.Termcolors::operator=(palette);
    }
}

void
ThemeSettings::setDircolors(const QString &dircolors)
{
    if (m_dircolors != dircolors) {
        m_dircolors = dircolors;
        m_content.Dircolors::operator=(dircolors);
    }
}

void
ThemeSettings::setLesser(bool lesser)
{
    m_lesser = lesser;
}
