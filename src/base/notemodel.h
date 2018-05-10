// Copyright © 2018 TermySequence LLC
//
// SPDX-License-Identifier: GPL-2.0-only

#pragma once

#include "note.h"

#include <QAbstractTableModel>
#include <QSortFilterProxyModel>
#include <QTableView>
#include <QVector>
#include <deque>

#define NOTE_COLUMN_MARK       0
#define NOTE_COLUMN_ROW        1
#define NOTE_COLUMN_USER       2
#define NOTE_COLUMN_HOST       3
#define NOTE_COLUMN_STARTED    4
#define NOTE_COLUMN_TEXT       5
#define NOTE_N_COLUMNS         6

// Note: must match job roles
#define NOTE_ROLE_ICON      Qt::ItemDataRole(Qt::UserRole + 1)
#define NOTE_ROLE_CODING    Qt::ItemDataRole(Qt::UserRole + 2)
#define NOTE_ROLE_REGION    Qt::ItemDataRole(Qt::UserRole + 3)
#define NOTE_ROLE_TERM      Qt::ItemDataRole(Qt::UserRole + 4)
#define NOTE_ROLE_NOTE      Qt::ItemDataRole(Qt::UserRole + 5)

#define NOTE_CODINGP(i) \
    static_cast<Tsq::Unicoding*>(i.data(NOTE_ROLE_CODING).value<void*>())
#define NOTE_TERMP(i) \
    static_cast<TermInstance*>(i.data(NOTE_ROLE_TERM).value<QObject*>())
#define NOTE_NOTEP(i) \
    static_cast<TermNote*>(i.data(NOTE_ROLE_NOTE).value<void*>())

QT_BEGIN_NAMESPACE
class QHeaderView;
class QMenu;
class QSvgRenderer;
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
class NoteModel final: public QAbstractTableModel
{
    Q_OBJECT

private:
    // Kept sorted by region start time
    std::deque<TermNote> m_notes;

    QVector<std::pair<size_t,InfoAnimation*>> m_animations;
    QSvgRenderer *m_renderer;

    size_t m_limit;

    void clearAnimations();
    void startAnimation(size_t row);

private slots:
    void handleTermAdded(TermInstance *term);
    void handleTermRemoved(TermInstance *term);

    void handleTermChanged();
    void handleNoteChanged(TermInstance *term, Region *region);
    void handleNoteDeleted(TermInstance *term, Region *region);

    void handleAnimation(intptr_t row);
    void handleAnimationFinished();

    void relimit();

public:
    NoteModel(TermListener *parent);

    void invalidateNotes(int64_t started);
    void removeClosedTerminals();

public:
    int columnCount(const QModelIndex &parent) const;
    int rowCount(const QModelIndex &parent) const;
    bool hasChildren(const QModelIndex &parent) const;
    bool hasIndex(int row, int column, const QModelIndex &parent) const;
    QModelIndex index(int row, int column, const QModelIndex &parent) const;

    QVariant headerData(int section, Qt::Orientation orientation, int role) const;
    QVariant data(const QModelIndex &index, int role) const;
    Qt::ItemFlags flags(const QModelIndex &index) const;
};

//
// Filter
//
class NoteFilter final: public QSortFilterProxyModel
{
    Q_OBJECT

private:
    NoteModel *m_model;

    QString m_search;
    QSet<Tsq::Uuid> m_whitelist;
    QSet<Tsq::Uuid> m_blacklist;
    bool m_haveWhitelist = false;
    bool m_haveBlacklist = false;

private slots:
    void setSearchString(const QString &str);

protected:
    bool filterAcceptsRow(int row, const QModelIndex &parent) const;

public:
    NoteFilter(NoteModel *model, QObject *parent);

    inline NoteModel* model() { return m_model; }

    void setWhitelist(const Tsq::Uuid &id);
    void addWhitelist(const Tsq::Uuid &id);
    void emptyWhitelist();
    void addBlacklist(const Tsq::Uuid &id);
    void resetFilter();
};

//
// View
//
class NoteView final: public QTableView
{
    Q_OBJECT

private:
    NoteFilter *m_filter;
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
    NoteView(NoteFilter *filter, ToolWidget *parent);

    void action(int type, const TermNote *note);
    void contextMenu(QPoint point);

    void selectFirst();
    void selectPrevious();
    void selectNext();
    void selectLast();

    void restoreState(int index, bool setting[3]);
    void saveState(int index, const bool setting[2]);
};
