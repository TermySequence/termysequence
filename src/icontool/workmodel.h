// Copyright © 2018 TermySequence LLC
//
// SPDX-License-Identifier: GPL-2.0-only

#pragma once

#include "thread.h"

#include <QAbstractTableModel>
#include <QSortFilterProxyModel>
#include <QTableView>
#include <QList>

struct WorkRecord;
struct InstallRecord;

class WorkModel final: public QAbstractTableModel, public IconWorker
{
    Q_OBJECT

private:
    QList<WorkRecord*> m_list;
    QString m_path;
    QString m_target;

    bool writeIndex(InstallRecord *irec) const;
    bool doHelper(const WorkRecord *rec, InstallRecord *irec) const;
    void doUpdate(IconThread *thread) const;
    void doInstall(IconThread *thread) const;

private slots:
    void handleIconsLoaded();

signals:
    void finished();

public:
    WorkModel(QObject *parent);
    ~WorkModel();

    void setIcon(int row, const QString &key);
    void save();
    void makeUpdate();
    void makeInstall();

    // Model functions
    int columnCount(const QModelIndex &parent = QModelIndex()) const;
    int rowCount(const QModelIndex &parent = QModelIndex()) const;
    QModelIndex index(int row, int column, const QModelIndex &parent = QModelIndex()) const;

    QVariant headerData(int section, Qt::Orientation orientation, int role) const;
    QVariant data(const QModelIndex &index, int role) const;
    Qt::ItemFlags flags(const QModelIndex &index) const;

    // Thread functions
    void run(IconThread *thread);
};

//
// Filter
//
class WorkFilter final: public QSortFilterProxyModel
{
private:
    QString m_search;

protected:
    bool filterAcceptsRow(int row, const QModelIndex &parent) const;

public:
    WorkFilter(QObject *parent);

    inline const auto& search() const { return m_search; }
    void setSearch(const QString &search);
};

//
// View
//
class WorkView final: public QTableView
{
protected:
    void contextMenuEvent(QContextMenuEvent *event);

public:
    WorkView();

    void setIcon();
    void unsetIcon();
};

extern WorkModel *g_workmodel;
extern WorkFilter *g_workfilter;
extern WorkView *g_workview;
