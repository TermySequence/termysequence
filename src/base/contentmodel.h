// Copyright © 2018 TermySequence LLC
//
// SPDX-License-Identifier: GPL-2.0-only

#pragma once

#include "lib/types.h"
#include "url.h"

#include <QAbstractTableModel>
#include <QTableView>
#include <QHash>
#include <QPixmap>
#include <QBuffer>
#include <QMovie>

#define CONTENT_COLUMN_NAME        0
#define CONTENT_COLUMN_TYPE        1
#define CONTENT_COLUMN_SIZE        2
#define CONTENT_COLUMN_REFCOUNT    3
#define CONTENT_N_COLUMNS          4

QT_BEGIN_NAMESPACE
class QHeaderView;
class QMenu;
QT_END_NAMESPACE
class Region;
class InfoAnimation;
class TermInstance;

struct TermMovie
{
    QByteArray data;
    QBuffer buffer;
    QMovie movie;
};

struct TermContent
{
    unsigned refcount = 1;
    unsigned moviecount = 0;
    unsigned size;
    bool isinline;
    bool enabled = false;
    bool loaded = false;
    QString id;
    TermUrl tu;
    index_t row;

    QPixmap pixmap;
    TermMovie *movie = nullptr;

    InfoAnimation *animation;
};

struct TermContentPopup
{
    QString id;
    TermUrl tu;
    bool isimage, isshown;
    bool addTermMenu;
};

//
// Tracker
//
class ContentTracker final: public QObject
{
    Q_OBJECT

private:
    QHash<QString,TermContent*> m_contentMap;
    QList<TermContent*> m_contentList;

private slots:
    void handleAnimation(intptr_t row);

signals:
    void contentUpdated(int row, bool bgonly);
    void contentAdding(int row);
    void contentAdded();
    void contentRemoving(int row);
    void contentRemoved(TermContent *content);
    void contentFetching(TermContent *content);

public:
    ContentTracker(QObject *parent);
    ~ContentTracker();

    inline int size() const { return m_contentList.size(); }
    inline TermContent* content(int row) const
    { return m_contentList.at(row); }
    inline TermContent* content(const QString &id) const
    { return m_contentMap.value(id); }
    inline int contentRow(TermContent *content) const
    { return m_contentList.indexOf(content); }

    inline QPixmap pixmap(const QString &id) const
    {
        TermContent *c = content(id);
        return c ? c->pixmap : QPixmap();
    }

    TermContent* addContent(const QString &id, const Region *region);
    void putContent(const QString &id);

    void startMovie(TermContent *content);
    void stopMovie(TermContent *content);
};

//
// Model
//
class ContentModel final: public QAbstractTableModel
{
    Q_OBJECT

private:
    ContentTracker *m_tracker;

private slots:
    void handleTrackerDestroyed();
    void handleContentUpdated(int row, bool bgonly);
    void handleContentAdding(int row);
    void handleContentRemoving(int row);

public:
    ContentModel(ContentTracker *tracker, QObject *parent);

    QModelIndex index(const QString &id) const;

public:
    int columnCount(const QModelIndex &parent) const;
    int rowCount(const QModelIndex &parent) const;
    QModelIndex index(int row, int column, const QModelIndex &parent) const;

    QVariant headerData(int section, Qt::Orientation orientation, int role) const;
    QVariant data(const QModelIndex &index, int role) const;
    Qt::ItemFlags flags(const QModelIndex &index) const;
};

//
// View
//
class ContentView final: public QTableView
{
private:
    ContentModel *m_model;
    TermInstance *m_term;

protected:
    void contextMenuEvent(QContextMenuEvent *event);

public:
    ContentView(ContentModel *model, TermInstance *term);

    void selectId(const QString &id);
};
