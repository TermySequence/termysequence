// Copyright © 2018 TermySequence LLC
//
// SPDX-License-Identifier: GPL-2.0-only

#include "common.h"
#include "app/attrbase.h"
#include "settings.h"

#define SK_MAINGEOM A("Window/MainGeometry")
#define SK_LOGGEOM A("Window/LogGeometry")
#define SK_SIZES A("Theme/Sizes")
#define SK_SOURCES A("Theme/Sources")
#define SK_REPO A("Theme/Repo")
#define SK_HIDDEN A("Model/Hidden")
#define SK_FILTER A("Model/Filter")
#define SK_KEYCOL A("Output/KeyColumn")
#define SK_INSTDIR A("Output/InstallDir")

#define REPO_NAME ABBREV_NAME "-icon-theme"

IconSettings *g_settings;

const struct {
    const char *name, *path;
    bool needsvg;
} s_defSources[] = {
    { "adwaita", "adwaita-icon-theme/Adwaita", 0 },
    { "oxygen", "oxygen-icons", 1 },
    { ABBREV_NAME, ABBREV_NAME "-icon-theme/icons/default", 0 },
    { NULL }
};

IconSettings::IconSettings(QObject *parent) :
    QSettings(parent)
{
    // Load settings
    QString githome = getenv("HOME");
    QStringList list;

    githome += A("/git/");

    list = value(SK_SOURCES).toStringList();
    for (int i = 0, n = list.size(); i < n; i += 3) {
        IconSource source = { list[i], list[i + 1], list[i + 2].toInt() != 0 };
        m_sources.insert(source.name, source);
    }
    for (const auto *i = s_defSources; i->name; ++i)
        if (!m_sources.contains(i->name)) {
            IconSource source = { i->name, githome + i->path, i->needsvg };
            m_sources.insert(source.name, source);
        }

    m_repo = githome + PROJECT_NAME;
    m_repo = value(SK_REPO, m_repo).toString();

    list.clear();
    list << "22" << "32" << "48";
    for (const auto &str: value(SK_SIZES, list).toStringList())
        m_sizes.insert(str.toInt());

    m_filter = value(SK_FILTER).toBool();
    m_hidden = QSet<QString>::fromList(value(SK_HIDDEN).toStringList());
    m_keycol = value(SK_KEYCOL, 84).toInt();

    m_installdir = L("/usr/local/share/" APP_NAME "/icons/default");
    m_installdir = value(SK_INSTDIR, m_installdir).toString();
}

// Setup
QString
IconSettings::workfile() const
{
    return repo() + A("/src/app/icons.h");
}

QString
IconSettings::distdir() const
{
    return m_sources.value(ABBREV_NAME).path;
}

void
IconSettings::saveSetup()
{
    QStringList list;
    for (const auto &i: m_sources)
        list << i.name << i.path << QString::number(i.needsvg);

    setValue(SK_SOURCES, list);
    setValue(SK_REPO, m_repo);
    setValue(SK_INSTDIR, m_installdir);
    sync();
}

// Icon view
const int IconSettings::allSizes[] = { 22, 24, 32, 48, 64, 96, 128, 256, 512, 0 };

void
IconSettings::setSizes(const QSet<int> &sizes)
{
    QStringList list;
    for (int size: sizes)
        list.append(QString::number(size));
    setValue(SK_SIZES, list);
    emit sizesChanged(m_sizes = sizes);
}

void
IconSettings::setHidden(const QSet<QString> &hidden)
{
    QStringList list = hidden.toList();
    setValue(SK_HIDDEN, list);
    m_hidden = hidden;
}

void
IconSettings::setFilter(bool filter)
{
    setValue(SK_FILTER, filter);
    emit filterChanged(m_filter = filter);
}

// Misc
QByteArray
IconSettings::windowGeometry() const
{
    return value(SK_MAINGEOM).toByteArray();
}

void
IconSettings::setWindowGeometry(const QByteArray &geometry)
{
    setValue(SK_MAINGEOM, geometry);
}

QByteArray
IconSettings::logGeometry() const
{
    return value(SK_LOGGEOM).toByteArray();
}

void
IconSettings::setLogGeometry(const QByteArray &geometry)
{
    setValue(SK_LOGGEOM, geometry);
}
