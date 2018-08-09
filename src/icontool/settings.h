// Copyright © 2018 TermySequence LLC
//
// SPDX-License-Identifier: GPL-2.0-only

#include <QSettings>
#include <QMap>
#include <QSet>

struct IconSource {
    QString name;
    QString path;
    bool needsvg;
};

class IconSettings final: public QSettings
{
    Q_OBJECT

private:
    QString m_repo;
    QMap<QString,IconSource> m_sources;
    QString m_installdir;

    QSet<int> m_sizes;
    QSet<QString> m_hidden;
    bool m_filter;

    int m_keycol;

public:
    // Setup
    inline auto repo() const { return m_repo; }
    inline void setRepo(const QString &repo) { m_repo = repo; }
    QString workfile() const;
    QString distdir() const;

    inline const auto& sources() const { return m_sources; }
    inline void setSources(QMap<QString,IconSource> &&sources)
    { m_sources = std::move(sources); }

    inline auto installdir() const { return m_installdir; }
    inline void setInstallDir(const QString &installdir) { m_installdir = installdir; }

    // Icon view
    static const int allSizes[];

    inline auto sizes() const { return m_sizes; }
    void setSizes(const QSet<int> &sizes);

    inline auto filter() const { return m_filter; }
    void setFilter(bool filter);

    inline auto hidden() const { return m_hidden; }
    void setHidden(const QSet<QString> &hidden);

    // Misc
    inline int keycol() const { return m_keycol; }

    QByteArray windowGeometry() const;
    void setWindowGeometry(const QByteArray &geometry);

    QByteArray logGeometry() const;
    void setLogGeometry(const QByteArray &geometry);

signals:
    void sizesChanged(QSet<int> sizes);
    void filterChanged(bool filter);

public:
    IconSettings(QObject *parent);

    void saveSetup();
};

extern IconSettings *g_settings;
