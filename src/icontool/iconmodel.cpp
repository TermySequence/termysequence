// Copyright © 2018 TermySequence LLC
//
// SPDX-License-Identifier: GPL-2.0-only

#include "common.h"
#include "app/attrbase.h"
#include "iconmodel.h"
#include "workmodel.h"
#include "viewitem.h"
#include "status.h"
#include "settings.h"

#include <QHeaderView>
#include <QDir>
#include <QMenu>
#include <QResizeEvent>

#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <cstdio>
#include <climits>
#include <algorithm>

#define OBJPROP_HIDDEN "opHidden"

IconModel *g_iconmodel;
IconView *g_iconview;

static bool
IconSorter(const IconRecord *a, const IconRecord *b)
{
    if (a->name != b->name)
        return a->name < b->name;

    return a->key < b->key;
}

void
IconModel::loadTheme(const IconSource &source, IconThread *thread)
{
    char buf[PATH_MAX] = {};
    unsigned count = 0;
    QRegularExpression re(A("^.*/16x16"));
    QString cmd = L("find %1/16x16 -name \\*.png -print").arg(source.path);
    FILE *ph = popen(pr(cmd), "re");
    struct stat info;

    while (fgets(buf, sizeof(buf) - 1, ph)) {
        size_t len = strlen(buf);
        if (len-- && buf[len] == '\n') buf[len] = '\0';
        QString path(buf), altpath;

        altpath = path;
        altpath.chop(4);
        altpath.replace(re, g_mtstr);

        if (path.endsWith(A(".symbolic.png")))
            continue;
        if (lstat(buf, &info) != 0 || !S_ISREG(info.st_mode))
            continue;

        IconRecord *rec = new IconRecord;
        rec->source = source.name;
        rec->needsvg = source.needsvg;
        rec->name = QFileInfo(path).baseName();
        rec->key = rec->source + altpath;
        rec->hidden = m_hidden.contains(rec->key);

        if (m_interactive)
            rec->icon = QIcon(path);

        altpath = path;
        altpath.replace(A("/16x16/"), L("/%1x%1/"));
        rec->pathfmt = altpath;

        QStringList sizes(A("16"));
        for (int i = 0; IconSettings::allSizes[i]; ++i) {
            int size = IconSettings::allSizes[i];

            if (stat(pr(altpath.arg(size)), &info) == 0) {
                rec->sizes.insert(size);
                sizes.append(QString::number(size));
            }
        }

        rec->tooltip = rec->key + A(" (") + sizes.join(' ') + ')';
        m_all.append(rec);
        m_map.insert(rec->key, rec);
        ++count;
        thread->incProgress();
    }

    g_status->log(L("loaded %1 icons from source %2").arg(count).arg(source.name));
    m_themeCounts.insert(source.name, count);
    pclose(ph);
}

void
IconModel::run(IconThread *thread)
{
    g_status->log("loading icons...");

    for (const auto &i: g_settings->sources())
        loadTheme(i, thread);

    std::sort(m_all.begin(), m_all.end(), IconSorter);
}

IconModel::IconModel(unsigned ncols, QObject *parent) :
    QAbstractTableModel(parent),
    m_ncols(ncols)
{
    m_interactive = parent->inherits("QApplication");
    m_sizes = g_settings->sizes();
    m_hidden = g_settings->hidden();
    m_filter = g_settings->filter();
}

void
IconModel::load(bool interactive)
{
    auto *thread = new IconThread(this, this, !interactive);
    thread->setMsg(A("Loading icons: %1"));
    connect(thread, SIGNAL(finished()), SLOT(postLoad()));
    thread->start();
}

void
IconModel::postLoad()
{
    computeFilter();
    computeSizes();
    connect(g_settings, &IconSettings::sizesChanged, this, &IconModel::setSizes);
    connect(g_settings, &IconSettings::filterChanged, this, &IconModel::setFilter);

    g_status->update();
    emit loaded();
}

IconModel::~IconModel()
{
    forDeleteAll(m_all);
}

void
IconModel::computeSizes()
{
    m_invalidCount = 0;
    for (auto *rec: m_all)
        m_invalidCount += (rec->invalid = !rec->sizes.contains(m_sizes));

    g_status->update();

    int nrows = rowCount();
    if (nrows > 0) {
        QModelIndex start = createIndex(0, 0);
        QModelIndex end = createIndex(nrows - 1, m_ncols - 1);
        emit dataChanged(start, end, QVector<int>(1, Qt::BackgroundRole));
    }
}

void
IconModel::computeFilter()
{
    beginResetModel();
    m_list = m_all;
    if (m_filter) {
        for (auto i = m_list.begin(); i != m_list.end(); ) {
            if ((*i)->hidden)
                i = m_list.erase(i);
            else
                ++i;
        }
    }
    if (!m_search.isEmpty()) {
        for (auto i = m_list.begin(); i != m_list.end(); ) {
            if (!(*i)->name.contains(m_search, Qt::CaseInsensitive))
                i = m_list.erase(i);
            else
                ++i;
        }
    }
    endResetModel();
}

void
IconModel::setSizes(QSet<int> sizes)
{
    m_sizes = sizes;
    computeSizes();
}

void
IconModel::setFilter(bool filter)
{
    m_filter = filter;
    computeFilter();
}

void
IconModel::setSearch(const QString &search)
{
    m_search = search;
    computeFilter();
}

void
IconModel::setHidden(const QModelIndex &index, bool hidden)
{
    auto *rec = (IconRecord *)index.internalPointer();
    if (rec) {
        if (hidden) {
            m_hidden.insert(rec->key);
            rec->hidden = true;
        } else {
            m_hidden.remove(rec->key);
            rec->hidden = false;
        }

        g_status->update();

        if (m_filter) {
            beginResetModel();
            m_list.removeOne(rec);
            endResetModel();
        } else {
            emit dataChanged(index, index, QVector<int>(1, Qt::BackgroundRole));
        }

        g_settings->setHidden(m_hidden);
    }
}

QIcon
IconModel::lookupIcon(const QString &key) const
{
    const auto *rec = m_map.value(key);
    if (rec) {
        return rec->icon;
    }
    else if (!key.isEmpty()) {
        g_status->log(L("warning: no icon found for key '%1'").arg(key), 1);
        g_status->reportLoadIssue();
    }
    return QIcon();
}

/*
 * Model functions
 */
int
IconModel::columnCount(const QModelIndex &parent) const
{
    return m_ncols;
}

int
IconModel::rowCount(const QModelIndex &parent) const
{
    return (m_list.size() / m_ncols) + !!(m_list.size() % m_ncols);
}

QModelIndex
IconModel::index(int row, int column, const QModelIndex &) const
{
    int pos = column * rowCount() + row;

    if (pos >= 0 && pos < m_list.size())
        return createIndex(row, column, (void*)m_list.at(pos));
    else
        return QModelIndex();
}

QVariant
IconModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    return QVariant();
}

QVariant
IconModel::data(const QModelIndex &index, int role) const
{
    const auto *rec = (const IconRecord *)index.internalPointer();
    if (rec)
        switch (role) {
        case Qt::BackgroundRole:
            if (rec->hidden)
                return QColor::fromRgb(0x9f9f9f);
            if (rec->invalid)
                return QColor::fromRgb(0xffbbbb);
            break;
        case Qt::DisplayRole:
            return rec->name;
        case Qt::DecorationRole:
            return rec->icon;
        case Qt::ToolTipRole:
            return rec->tooltip;
        }

    return QVariant();
}

Qt::ItemFlags
IconModel::flags(const QModelIndex &index) const
{
    return index.internalPointer() ?
        Qt::ItemIsSelectable | Qt::ItemIsEnabled :
        Qt::NoItemFlags;
}

//
// View
//
IconView::IconView()
{
    QItemSelectionModel *m = selectionModel();
    setModel(g_iconmodel);
    delete m;

    setShowGrid(false);
    setSelectionBehavior(QAbstractItemView::SelectItems);
    setSelectionMode(QAbstractItemView::ExtendedSelection);
    verticalHeader()->setVisible(false);
    horizontalHeader()->setSectionResizeMode(QHeaderView::Fixed);
    horizontalHeader()->setVisible(false);

    setItemDelegate(new IconItemDelegate(this));
    resizeRowsToContents();
}

void
IconView::resizeEvent(QResizeEvent *event)
{
    int n = g_iconmodel->columnCount();
    int width = event->size().width() / n;

    for (int i = 0; i < n; ++i)
        horizontalHeader()->resizeSection(i, width);
}

void
IconView::hideIcons()
{
    bool hidden = sender()->property(OBJPROP_HIDDEN).toBool();

    for (auto &index: selectionModel()->selectedIndexes())
        g_iconmodel->setHidden(index, hidden);
}

void
IconView::contextMenuEvent(QContextMenuEvent *event)
{
    QMenu *menu = new QMenu(this);
    QAction *a;

    a = menu->addAction(A("Hide"), this, &IconView::hideIcons);
    a->setProperty(OBJPROP_HIDDEN, true);

    a = menu->addAction(A("Show"), this, &IconView::hideIcons);
    a->setProperty(OBJPROP_HIDDEN, false);

    menu->addSeparator();
    menu->addAction(A("Set Icon"), g_workview, &WorkView::setIcon);

    menu->popup(event->globalPos());
    event->accept();
}
