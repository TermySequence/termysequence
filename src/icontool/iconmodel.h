// Copyright © 2018 TermySequence LLC
//
// SPDX-License-Identifier: GPL-2.0-only

#pragma once

#include "thread.h"

#include <QAbstractTableModel>
#include <QTableView>
#include <QSet>
#include <QIcon>
#include <QList>
#include <QMap>

//
// Record
//
struct IconRecord
{
    QSet<int> sizes;
    QString source;
    QString name;
    QString key;
    QString pathfmt;
    QString tooltip;
    QIcon icon;
    bool invalid;
    bool hidden;
    bool needsvg;
};

struct IconSource;

//
// Model
//
class IconModel final: public QAbstractTableModel, public IconWorker
{
    Q_OBJECT

private:
    QList<IconRecord*> m_all, m_list;
    QMap<QString,IconRecord*> m_map;

    unsigned m_ncols;
    bool m_filter;
    bool m_interactive;
    QSet<int> m_sizes;
    QSet<QString> m_hidden;
    QString m_search;

    QMap<QString,unsigned> m_themeCounts;
    int m_invalidCount;

    void loadTheme(const IconSource &source, IconThread *thread);
    void computeSizes();
    void computeFilter();

private slots:
    void postLoad();

signals:
    void loaded();

public:
    IconModel(unsigned ncols, QObject *parent);
    ~IconModel();

    inline const auto& sizes() const { return m_sizes; }
    void setSizes(QSet<int> sizes);

    inline bool filter() const { return m_filter; }
    inline const auto& search() const { return m_search; }
    void setFilter(bool filter);
    void setHidden(const QModelIndex &index, bool hidden);
    void setSearch(const QString &search);

    inline const auto* lookup(const QString &key) const { return m_map.value(key); }
    QIcon lookupIcon(const QString &key) const;

    unsigned themeCount(const QString &name) const { return m_themeCounts.value(name); }
    inline unsigned invalidCount() const { return m_invalidCount; }
    inline unsigned hiddenCount() const { return m_hidden.size(); }

    // Model functions
    int columnCount(const QModelIndex &parent = QModelIndex()) const;
    int rowCount(const QModelIndex &parent = QModelIndex()) const;
    QModelIndex index(int row, int column, const QModelIndex &parent = QModelIndex()) const;

    QVariant headerData(int section, Qt::Orientation orientation, int role) const;
    QVariant data(const QModelIndex &index, int role) const;
    Qt::ItemFlags flags(const QModelIndex &index) const;

    // Thread functions
    void load(bool interactive);
    void run(IconThread *thread);
};

//
// View
//
class IconView final: public QTableView
{
protected:
    void contextMenuEvent(QContextMenuEvent *event);
    void resizeEvent(QResizeEvent *event);

public:
    IconView();

    void hideIcons();
};

extern IconModel *g_iconmodel;
extern IconView *g_iconview;
