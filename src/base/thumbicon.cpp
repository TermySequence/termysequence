// Copyright © 2018 TermySequence LLC
//
// SPDX-License-Identifier: GPL-2.0-only

#include "common.h"
#include "app/attrbase.h"
#include "app/config.h"
#include "thumbicon.h"
#include "settings/settings.h"
#include "settings/global.h"
#include "settings/state.h"

#include <QPainter>
#include <QHash>
#include <QSet>
#include <QDir>
#include <QFileInfo>
#include <QSharedPointer>
#include <QSvgRenderer>
#include <QPainter>
#include <unordered_map>
#include <cassert>

//
// Category Manager
//
class RendererManager
{
private:
    QHash<QString,QSharedPointer<QSvgRenderer>> m_rmap;
    QHash<QString,QIcon> m_imap;
    QDir m_userDir;
    QDir m_systemDir;
    QDir m_resourceDir;
    StateSettingsKey m_settingsKey;
    bool m_haveUserDir;
    bool m_haveSystemDir;

    QSet<QString> m_allset;
    QStringList m_all;
    QStringList m_favorites;
    ThumbIcon::SIVector m_menulist;

    void computeFavorites();

public:
    RendererManager(const char *subdir, StateSettingsKey key = InvalidStateKey);

    QSvgRenderer *getRenderer(const QString &name);
    QPixmap getPixmap(const QString &name, int size);
    QIcon getIcon(const QString &name);
    QString getPath() const;
    bool loadIcon(const QString &name, const QByteArray &data);

    void scan();
    ThumbIcon::SIVector allIcons();
    inline const QStringList& allNames();
    inline const auto& favorites() const { return m_menulist; }
    void addFavorite(const QString &name);
};

RendererManager::RendererManager(const char *subdir, StateSettingsKey key) :
    m_settingsKey(key),
    m_haveUserDir(false)
{
    // Compiled-in resource dir
    QString path = L(":/dist/images/%1/");
    m_resourceDir = QDir(path.arg(subdir));
    m_resourceDir.setNameFilters(QStringList(L("*.svg")));
    m_resourceDir.setFilter(QDir::Files|QDir::Readable);
    m_resourceDir.setSorting(QDir::Name);

    // System data dir
    path = L(PREFIX "/share/" APP_NAME "/images/%1/");
    m_systemDir = QDir(path.arg(subdir));
    if ((m_haveSystemDir = m_systemDir.exists())) {
        m_systemDir.setNameFilters(QStringList(L("*.svg")));
        m_systemDir.setFilter(QDir::Files|QDir::Readable);
        m_systemDir.setSorting(QDir::Name);
    }

    // User data dir
    path = g_settings->dataPath();
    if (!path.isEmpty()) {
        path.append(A("/images/%2/"));
        m_userDir = QDir(path.arg(subdir));
        if ((m_haveUserDir = m_userDir.isAbsolute() && m_userDir.exists())) {
            m_userDir.setNameFilters(QStringList(L("*.svg")));
            m_userDir.setFilter(QDir::Files|QDir::Readable);
            m_userDir.setSorting(QDir::Name);
        }
    }

    // Default file
    assert(m_resourceDir.exists());
    assert(m_resourceDir.exists(L("default.svg")));
    getRenderer(TI_DEFAULT_NAME);
    m_rmap[g_mtstr] = m_rmap[TI_DEFAULT_NAME];

    // None icon
    m_imap[TI_NONE_NAME] = QIcon();
}

void
RendererManager::computeFavorites()
{
    QSet<QString> seen({ TI_NONE_NAME });
    int i = 0, n = g_global->menuSize();
    m_menulist.clear();

    for (const auto &str: m_favorites) {
        m_menulist.append(std::make_pair(str, getIcon(str)));
        seen.insert(str);
    }

    while (m_menulist.size() < n && i < m_all.size()) {
        QString str = m_all[i++];
        if (!seen.contains(str)) {
            m_menulist.push_back(std::make_pair(str, getIcon(str)));
            seen.insert(str);
        }
    }
}

void
RendererManager::scan()
{
    QFileInfoList files;
    QString basename;

    m_allset.clear();
    m_all.clear();

    if (m_haveUserDir) {
        m_userDir.refresh();
        files = m_userDir.entryInfoList();
        for (auto &i: qAsConst(files)) {
            basename = i.completeBaseName();
            m_allset.insert(basename);
            m_all.push_back(basename);
        }
    }
    if (m_haveSystemDir) {
        m_systemDir.refresh();
        files = m_systemDir.entryInfoList();
        for (auto &i: qAsConst(files)) {
            basename = i.completeBaseName();
            if (!m_allset.contains(basename)) {
                m_allset.insert(basename);
                m_all.push_back(basename);
            }
        }
    }
    files = m_resourceDir.entryInfoList();
    for (auto &i: qAsConst(files)) {
        basename = i.completeBaseName();
        if (!m_allset.contains(basename)) {
            m_allset.insert(basename);
            m_all.push_back(basename);
        }
    }

    if (m_settingsKey != InvalidStateKey) {
        m_favorites = g_state->fetchList(m_settingsKey);
        bool changed = false;

        for (auto i = m_favorites.begin(); i != m_favorites.end(); ) {
            if (!m_allset.contains(*i)) {
                i = m_favorites.erase(i);
                changed = true;
            } else {
                ++i;
            }
        }

        computeFavorites();

        if (changed) {
            g_state->storeList(m_settingsKey, m_favorites);
            emit g_settings->iconsChanged();
        }
    } else {
        m_allset.clear();
    }
}

ThumbIcon::SIVector
RendererManager::allIcons()
{
    scan();

    ThumbIcon::SIVector result;
    for (const auto &i: m_all) {
        result.append(std::make_pair(i, getIcon(i)));
    }
    return result;
}

const QStringList &
RendererManager::allNames()
{
    scan();
    return m_all;
}

void
RendererManager::addFavorite(const QString &name)
{
    if (name == TI_NONE_NAME || !m_allset.contains(name))
        return;

    if (m_favorites.contains(name))
        m_favorites.removeOne(name);

    while (m_favorites.size() >= g_global->menuSize())
        m_favorites.pop_back();

    m_favorites.prepend(name);
    computeFavorites();

    g_state->storeList(m_settingsKey, m_favorites);
    emit g_settings->iconsChanged();
}

QSvgRenderer *
RendererManager::getRenderer(const QString &rawname)
{
    // Sanitize name
    QString name = rawname.section('/', -1).section('\\', -1);

    auto i = m_rmap.constFind(name);
    if (i == m_rmap.cend())
    {
        QString file(name + A(".svg"));

        if (m_haveUserDir && m_userDir.exists(file)) {
            file = m_userDir.filePath(file);
            i = m_rmap.insert(name, QSharedPointer<QSvgRenderer>(new QSvgRenderer(file)));
        }
        else if (m_haveSystemDir && m_systemDir.exists(file)) {
            file = m_systemDir.filePath(file);
            i = m_rmap.insert(name, QSharedPointer<QSvgRenderer>(new QSvgRenderer(file)));
        }
        else if (m_resourceDir.exists(file)) {
            file = m_resourceDir.filePath(file);
            i = m_rmap.insert(name, QSharedPointer<QSvgRenderer>(new QSvgRenderer(file)));
        }
        else {
            i = m_rmap.insert(name, m_rmap[g_mtstr]);
        }
    }
    return i->data();
}

bool
RendererManager::loadIcon(const QString &name, const QByteArray &data)
{
    QSharedPointer<QSvgRenderer> renderer(new QSvgRenderer(data));
    if (renderer->isValid()) {
        m_rmap[name] = renderer;
        return true;
    }
    return false;
}

QPixmap
RendererManager::getPixmap(const QString &name, int size)
{
    QPixmap pixmap(size, size);
    pixmap.fill(Qt::transparent);

    QPainter painter(&pixmap);
    getRenderer(name)->render(&painter);
    return pixmap;
}

QIcon
RendererManager::getIcon(const QString &name)
{
    auto i = m_imap.constFind(name);
    if (i == m_imap.cend())
        i = m_imap.insert(name, getPixmap(name, PIXMAP_ICON_SIZE));

    return *i;
}

QString
RendererManager::getPath() const
{
    return m_userDir.path();
}

//
// Static methods
//
static RendererManager *s_managers[ThumbIcon::NIconType];

const QStringList &
ThumbIcon::getAllNames(IconType type)
{
    return s_managers[type]->allNames();
}

ThumbIcon::SIVector
ThumbIcon::getAllIcons(IconType type)
{
    return s_managers[type]->allIcons();
}

const ThumbIcon::SIVector &
ThumbIcon::getFavoriteIcons(IconType type)
{
    return s_managers[type]->favorites();
}

void
ThumbIcon::setFavoriteIcon(IconType type, const QString &name)
{
    s_managers[type]->addFavorite(name);
}

QSvgRenderer *
ThumbIcon::getRenderer(IconType type, const QString &name)
{
    return s_managers[type]->getRenderer(name);
}

QPixmap
ThumbIcon::getPixmap(IconType type, const QString &name, int size)
{
    return s_managers[type]->getPixmap(name, size);
}

QIcon
ThumbIcon::getIcon(IconType type, const QString &name)
{
    return s_managers[type]->getIcon(name);
}

QString
ThumbIcon::getPath(IconType type)
{
    return s_managers[type]->getPath();
}

bool
ThumbIcon::loadIcon(IconType type, const QString &name, const QByteArray &data)
{
    return s_managers[type]->loadIcon(name, data);
}

static std::unordered_map<std::string,QIcon> *s_iconcache;
QStringList ThumbIconCache::allThemes;

QIcon
ThumbIcon::fromTheme(const std::string &name)
{
    if (!s_iconcache)
        return QIcon();

    auto i = s_iconcache->find(name);
    if (i != s_iconcache->end())
        return i->second;

    QIcon icon = QIcon::fromTheme(QString::fromStdString(name));
    s_iconcache->emplace(name, icon);
    return icon;
}

QString
ThumbIcon::getThemePath(const std::string &name, const QLatin1String &size)
{
    QString theme = g_global->iconTheme();
    if (!theme.isEmpty()) {
        QString path = L(PREFIX "/share/" APP_NAME "/icons/%1/%2x%2/%3.png")
            .arg(theme, size, QString::fromStdString(name));

        if (QFileInfo(path).isReadable())
            return path;
    }
    return g_mtstr;
}

void
ThumbIconCache::initialize()
{
    QString path(L(PREFIX "/share/" APP_NAME "/icons"));
    QDir dir(path, g_mtstr, QDir::Name, QDir::Dirs|QDir::NoDotAndDotDot);

    allThemes = dir.entryList();
    QString theme = g_global->iconTheme();

    if (allThemes.contains(theme)) {
        s_iconcache = new std::unordered_map<std::string,QIcon>;
        QIcon::setThemeSearchPaths(QStringList(path));
        QIcon::setThemeName(theme);
    }

    allThemes.append(TI_NONE_NAME);
}

void
ThumbIcon::initialize()
{
    s_managers[TerminalType] = new RendererManager("terminal", TermIconsKey);
    s_managers[ServerType] = new RendererManager("server", ServerIconsKey);
    s_managers[CommandType] = new RendererManager("command");
    s_managers[IndicatorType] = new RendererManager("indicator");
    s_managers[SemanticType] = new RendererManager("semantic");
    s_managers[AvatarType] = new RendererManager("avatar");
    s_managers[EmojiType] = new RendererManager("emoji");

    s_managers[TerminalType]->scan();
    s_managers[ServerType]->scan();
}

void
ThumbIcon::teardown()
{
    delete s_managers[EmojiType];
    delete s_managers[AvatarType];
    delete s_managers[SemanticType];
    delete s_managers[IndicatorType];
    delete s_managers[CommandType];
    delete s_managers[ServerType];
    delete s_managers[TerminalType];
    delete s_iconcache;
}

//
// Widget
//
ThumbIcon::ThumbIcon(IconType type, QWidget *parent) :
    QWidget(parent)
{
    m_renderer = m_renderers[0] = s_managers[type]->getRenderer(g_mtstr);
    m_renderers[1] = nullptr;
}

void
ThumbIcon::setIcon(const QString &name, IconType type)
{
    auto *r1 = s_managers[type]->getRenderer(name);

    if (m_renderers[1] != r1) {
        m_renderer = m_renderers[1] = r1;
        update();
    }
}

void
ThumbIcon::setIcons(const QString *i, const int *t)
{
    auto *r0 = s_managers[t[0]]->getRenderer(i[0]);
    auto *r1 = !i[1].isEmpty() ? s_managers[t[1]]->getRenderer(i[1]) : nullptr;

    m_renderers[0] = r0;
    m_renderers[1] = r1;

    auto *r = r1 ? r1 : r0;
    if (m_renderer != r) {
        m_renderer = r;
        update();
    }
}

void
ThumbIcon::paintEvent(QPaintEvent *)
{
    QPainter painter(this);
    m_renderer->render(&painter);
}
