// Copyright © 2018 TermySequence LLC
//
// SPDX-License-Identifier: GPL-2.0-only

#pragma once

#include <QAbstractTableModel>
#include <QSortFilterProxyModel>
#include <QTableView>
#include <QMap>
#include <QRgb>

// Note: order matters
#define FILE_COLUMN_MODE        0
#define FILE_COLUMN_USER        1
#define FILE_COLUMN_GROUP       2
#define FILE_COLUMN_SIZE        3
#define FILE_COLUMN_MTIME       4
#define FILE_COLUMN_GIT         5
#define FILE_COLUMN_NAME        6
#define FILE_N_COLUMNS          7

#define FILE_ROLE_FILE     Qt::ItemDataRole(Qt::UserRole + 1)
#define FILE_ROLE_TERM     Qt::ItemDataRole(Qt::UserRole + 2)

#define FILE_FILEP(i) \
    static_cast<TermFile*>(i.data(FILE_ROLE_FILE).value<void*>())
#define FILE_TERMP(i) \
    static_cast<TermInstance*>(i.data(FILE_ROLE_TERM).value<QObject*>())

QT_BEGIN_NAMESPACE
class QHeaderView;
class QMenu;
QT_END_NAMESPACE
class TermManager;
class TermInstance;
class ServerInstance;
class FileTracker;
struct TermDirectory;
struct TermFile;
class FileWidget;
class FileViewItem;
class FileNameItem;
class InfoAnimation;

//
// Model
//
class FileModel final: public QAbstractTableModel
{
    Q_OBJECT

private:
    TermInstance *m_term = nullptr;
    FileTracker *m_files = nullptr;
    FileNameItem *m_nameitem;

    QMap<int,InfoAnimation*> m_animations;
    int m_timerId = 0;
    bool m_loaded = true;
    bool m_animating = false;

    void adjustAnimations(int bound, int delta);
    void startAnimation(int idx);
    void clearAnimations();
    void setBlinkEffect();

private slots:
    void handleTermActivated(TermInstance *term);
    void handlePaletteChanged();

    void handleDirectoryChanging();
    void handleDirectoryChanged(bool attronly);
    void handleFileUpdated(int row, unsigned changes);
    void handleFileAdding(int row);
    void handleFileAdded(int row);
    void handleFileRemoving(int row);

    void handleAnimation(intptr_t row);
    void handleAnimationFinished();
    void handleBlinkTimer();

protected:
    void timerEvent(QTimerEvent *event);

signals:
    void termChanged(const TermDirectory *dir);
    void directoryChanged(const TermDirectory *dir);
    void metadataChanged(const TermDirectory *dir);

    void colorsChanged(QRgb bg, QRgb fg);
    void fontChanged(const QFont &font);

public:
    FileModel(TermManager *manager, FileNameItem *m_nameitem, QObject *parent);

    inline TermInstance* term() const { return m_term; }
    inline const FileTracker* files() const { return m_files; }
    inline bool animating() const { return m_animating; }

    QString dir() const;
    ServerInstance* server() const;

public:
    int columnCount(const QModelIndex &parent) const;
    int rowCount(const QModelIndex &parent) const;
    bool hasChildren(const QModelIndex &parent) const;
    bool hasIndex(int row, int column, const QModelIndex &parent) const;
    QModelIndex index(int row, int column, const QModelIndex &parent) const;

    static QString headerData(int section);
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
class FileFilter final: public QSortFilterProxyModel
{
    Q_OBJECT

private:
    FileModel *m_model;

    QString m_search;

    bool isSearchMatch(int row, const QModelIndex &parent) const;

protected:
    bool filterAcceptsRow(int row, const QModelIndex &parent) const;
    bool lessThan(const QModelIndex &left, const QModelIndex &right) const;

public:
    FileFilter(FileModel *model, QObject *parent);

    inline FileModel* model() { return m_model; }

public slots:
    void setSearchString(const QString &str);
};

//
// View
//
class FileView final: public QTableView
{
    Q_OBJECT

private:
    FileFilter *m_filter;
    FileViewItem *m_viewitem;
    FileNameItem *m_nameitem;
    FileWidget *m_parent;
    QHeaderView *m_header;
    QMenu *m_headerMenu = nullptr;

    QPoint m_dragStartPosition;
    bool m_clicking = false;
    void mouseAction(int action);

private slots:
    void refont(const QFont &font);

    void handleHeaderContextMenu(const QPoint &pos);
    void preshowHeaderContextMenu();
    void handleHeaderTriggered(bool checked);

    void handleActivity();

protected:
    void scrollContentsBy(int dx, int dy);
    void contextMenuEvent(QContextMenuEvent *event);
    void mouseDoubleClickEvent(QMouseEvent *event);
    void mousePressEvent(QMouseEvent *event);
    void mouseMoveEvent(QMouseEvent *event);
    void mouseReleaseEvent(QMouseEvent *event);

public:
    FileView(FileFilter *filter, FileNameItem *item, FileWidget *parent);

    void selectItem(int selected);

    void restoreState(int index, bool setting[2]);
    void saveState(int index, const bool setting[2]);
};
