// Copyright © 2018 TermySequence LLC
//
// SPDX-License-Identifier: GPL-2.0-only

#pragma once

#include "job.h"

#include <QAbstractTableModel>
#include <QSortFilterProxyModel>
#include <QTableView>
#include <QVector>
#include <deque>

#define JOB_COLUMN_MARK       0
#define JOB_COLUMN_ROW        1
#define JOB_COLUMN_USER       2
#define JOB_COLUMN_HOST       3
#define JOB_COLUMN_PATH       4
#define JOB_COLUMN_STARTED    5
#define JOB_COLUMN_DURATION   6
#define JOB_COLUMN_COMMAND    7
#define JOB_N_COLUMNS         8

// Note: must match note roles
#define JOB_ROLE_ICON      Qt::ItemDataRole(Qt::UserRole + 1)
#define JOB_ROLE_CODING    Qt::ItemDataRole(Qt::UserRole + 2)
#define JOB_ROLE_REGION    Qt::ItemDataRole(Qt::UserRole + 3)
#define JOB_ROLE_TERM      Qt::ItemDataRole(Qt::UserRole + 4)
#define JOB_ROLE_JOB       Qt::ItemDataRole(Qt::UserRole + 5)

#define JOB_CODINGP(i) \
    static_cast<Tsq::Unicoding*>(i.data(JOB_ROLE_CODING).value<void*>())
#define JOB_ICONP(i) \
    static_cast<QSvgRenderer*>(i.data(JOB_ROLE_ICON).value<QObject*>())
#define JOB_TERMP(i) \
    static_cast<TermInstance*>(i.data(JOB_ROLE_TERM).value<QObject*>())
#define JOB_JOBP(i) \
    static_cast<TermJob*>(i.data(JOB_ROLE_JOB).value<void*>())

QT_BEGIN_NAMESPACE
class QHeaderView;
class QMenu;
QT_END_NAMESPACE
class TermListener;
class TermManager;
class TermInstance;
class TermScrollport;
class Region;
class ToolWidget;
class MainWindow;
class InfoAnimation;
class JobViewItem;

//
// Model
//
class JobModel final: public QAbstractTableModel
{
    Q_OBJECT

private:
    // Kept sorted by region start time
    std::deque<TermJob> m_jobs;
    QVector<std::pair<size_t,InfoAnimation*>> m_animations;
    size_t m_limit;

    void clearAnimations();
    void startAnimation(size_t row);

private slots:
    void handleTermAdded(TermInstance *term);
    void handleTermRemoved(TermInstance *term);

    void handleTermChanged();
    void handleJobChanged(TermInstance *term, Region *region);

    void handleAnimation(intptr_t row);
    void handleAnimationFinished();

    void relimit();

public:
    JobModel(TermListener *parent);

    void invalidateJobs(int64_t started);
    void removeClosedTerminals();
    QString findCommand(const Tsq::Uuid &terminalId, regionid_t regionId) const;

public:
    int columnCount(const QModelIndex &parent) const;
    int rowCount(const QModelIndex &parent) const;
    bool hasChildren(const QModelIndex &parent) const;
    bool hasIndex(int row, int column, const QModelIndex &parent) const;
    QModelIndex index(int row, int column, const QModelIndex &parent) const;

    QVariant headerData(int section, Qt::Orientation orientation, int role) const;
    QVariant data(const QModelIndex &index, int role) const;
    Qt::ItemFlags flags(const QModelIndex &index) const;

    QStringList mimeTypes() const;
    QMimeData* mimeData(const QModelIndexList &indexes) const;
    bool dropMimeData(const QMimeData *data, Qt::DropAction action,
                      int row, int column, const QModelIndex &parent);
};

//
// Filter
//
class JobFilter final: public QSortFilterProxyModel
{
    Q_OBJECT

private:
    TermManager *m_manager;
    TermScrollport *m_scrollport = nullptr;
    JobModel *m_model;
    QMetaObject::Connection m_mocTerm;

    int64_t m_activeJobStarted = INVALID_WALLTIME;

    QString m_search;
    QSet<Tsq::Uuid> m_whitelist;
    QSet<Tsq::Uuid> m_blacklist;
    bool m_haveWhitelist = false;
    bool m_haveBlacklist = false;

private slots:
    void handleTermActivated(TermInstance *term, TermScrollport *scrollport);
    void handleActiveJobChanged(regionid_t activeJobId);

    void setSearchString(const QString &str);

protected:
    bool filterAcceptsRow(int row, const QModelIndex &parent) const;

public:
    JobFilter(JobModel *model, ToolWidget *parent);

    inline JobModel* model() { return m_model; }

    QVariant data(const QModelIndex &index, int role) const;

    void setWhitelist(const Tsq::Uuid &id);
    void addWhitelist(const Tsq::Uuid &id);
    void emptyWhitelist();
    void addBlacklist(const Tsq::Uuid &id);
    void resetFilter();
};

//
// View
//
class JobView final: public QTableView
{
    Q_OBJECT

private:
    JobFilter *m_filter;
    TermInstance *m_term = nullptr;
    MainWindow *m_window;

    QMetaObject::Connection m_mocTerm;

    JobViewItem *m_item;
    QHeaderView *m_header;
    QMenu *m_headerMenu = nullptr;

    bool m_autoscroll = false;

    bool m_clicking = false;
    QPoint m_dragStartPosition;
    void mouseAction(int action);

private slots:
    void recolor(QRgb bg, QRgb fg);
    void refont(const QString &fontstr);

    void handleTermActivated(TermInstance *term);
    void handleHeaderContextMenu(const QPoint &pos);
    void preshowHeaderContextMenu();
    void handleHeaderTriggered(bool checked);

    void handleAutoscroll();

protected:
    void contextMenuEvent(QContextMenuEvent *event);
    void mouseDoubleClickEvent(QMouseEvent *event);
    void mousePressEvent(QMouseEvent *event);
    void mouseMoveEvent(QMouseEvent *event);
    void mouseReleaseEvent(QMouseEvent *event);

public slots:
    void setAutoscroll(bool autoscroll);

public:
    JobView(JobFilter *filter, ToolWidget *parent);

    void action(int type, const TermJob *job);
    void contextMenu(QPoint point);

    void selectFirst();
    void selectPrevious();
    void selectNext();
    void selectLast();

    void restoreState(int index, bool setting[3]);
    void saveState(int index, const bool setting[2]);
};
