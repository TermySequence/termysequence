// Copyright © 2018 TermySequence LLC
//
// SPDX-License-Identifier: GPL-2.0-only

#include "common.h"
#include "app/attr.h"
#include "contentmodel.h"
#include "region.h"
#include "infoanim.h"
#include "term.h"
#include "listener.h"
#include "manager.h"
#include "mainwindow.h"

#include <QHeaderView>
#include <QMenu>
#include <QContextMenuEvent>

#define CONTENT_CONTENTP(i) \
    static_cast<TermContent*>(i.data(Qt::UserRole).value<void*>())

//
// Tracker
//
ContentTracker::ContentTracker(QObject *parent) : QObject(parent)
{
}

ContentTracker::~ContentTracker()
{
    forDeleteAll(m_contentList);
}

TermContent *
ContentTracker::addContent(const QString &id, const Region *region)
{
    TermContent *content = m_contentMap.value(id);
    if (content) {
        ++content->refcount;
        QString name = region->attributes.value(g_attr_CONTENT_NAME);
        if (!name.isEmpty()) {
            content->tu = TermUrl::parse(name);
        }
        content->row = region->startRow;
        emit contentUpdated(m_contentList.indexOf(content), false);
        content->animation->start();
        return nullptr; // not new
    }
    else {
        int row = m_contentList.size();
        content = new TermContent();
        content->size = region->attributes.value(g_attr_CONTENT_SIZE).toUInt();
        content->isinline = region->attributes.value(g_attr_CONTENT_INLINE) == A("1");
        content->id = id;
        content->tu = TermUrl::parse(region->attributes.value(g_attr_CONTENT_NAME));
        content->row = region->startRow;
        content->animation = new InfoAnimation(this, row);

        connect(content->animation, SIGNAL(animationSignal(intptr_t)),
                SLOT(handleAnimation(intptr_t)));

        emit contentAdding(row);
        m_contentMap.insert(id, content);
        m_contentList.append(content);
        emit contentAdded();
        content->animation->start();
        return content; // new item
    }
}

void
ContentTracker::putContent(const QString &id)
{
    TermContent *content = m_contentMap.value(id);
    if (content) {
        int row = m_contentList.indexOf(content);
        if (--content->refcount == 0) {
            emit contentRemoving(row);
            m_contentList.removeAt(row);
            m_contentMap.remove(id);

            // Adjust animation rows
            while (row < m_contentList.size()) {
                m_contentList.at(row)->animation->setData(row);
                ++row;
            }

            emit contentRemoved(content);
            delete content->animation;
            delete content->movie;
            delete content;
        }
        else {
            emit contentUpdated(row, false);
            content->animation->start();
        }
    }
}

void
ContentTracker::startMovie(TermContent *content)
{
    if (++content->moviecount == 1) {
        content->movie->movie.start();
    }
}

void
ContentTracker::stopMovie(TermContent *content)
{
    if (--content->moviecount == 0) {
        content->movie->movie.setPaused(true);
    }
}

void
ContentTracker::handleAnimation(intptr_t row)
{
    emit contentUpdated(row, true);
}

//
// Model
//
ContentModel::ContentModel(ContentTracker *tracker, QObject *parent) :
    QAbstractTableModel(parent),
    m_tracker(tracker)
{
    connect(tracker, SIGNAL(destroyed()), SLOT(handleTrackerDestroyed()));
    connect(tracker, SIGNAL(contentUpdated(int,bool)), SLOT(handleContentUpdated(int,bool)));
    connect(tracker, SIGNAL(contentAdding(int)), SLOT(handleContentAdding(int)));
    connect(tracker, &ContentTracker::contentAdded, this, &ContentModel::endInsertRows);
    connect(tracker, SIGNAL(contentRemoving(int)), SLOT(handleContentRemoving(int)));
    connect(tracker, &ContentTracker::contentRemoved, this, &ContentModel::endRemoveRows);
}

void
ContentModel::handleTrackerDestroyed()
{
    // beginResetModel(); Note: causes error messages
    m_tracker = nullptr;
    // endResetModel();
}

void
ContentModel::handleContentUpdated(int row, bool bgonly)
{
    QModelIndex start = createIndex(row, 0);
    QModelIndex end = createIndex(row, CONTENT_N_COLUMNS - 1);
    QVector<int> roles;

    if (bgonly)
        roles.append(Qt::BackgroundRole);

    emit dataChanged(start, end, roles);
}

void
ContentModel::handleContentAdding(int row)
{
    beginInsertRows(QModelIndex(), row, row);
}

void
ContentModel::handleContentRemoving(int row)
{
    beginRemoveRows(QModelIndex(), row, row);
}

/*
 * Model functions
 */
int
ContentModel::columnCount(const QModelIndex &) const
{
    return CONTENT_N_COLUMNS;
}

int
ContentModel::rowCount(const QModelIndex &) const
{
    return m_tracker ? m_tracker->size() : 0;
}

QModelIndex
ContentModel::index(int row, int column, const QModelIndex &) const
{
    if (m_tracker && row < m_tracker->size())
        return createIndex(row, column, (void*)m_tracker->content(row));
    else
        return QModelIndex();
}

QModelIndex
ContentModel::index(const QString &id) const
{
    TermContent *content;

    if (m_tracker && (content = m_tracker->content(id)))
        return createIndex(m_tracker->contentRow(content), 0, (void*)content);
    else
        return QModelIndex();
}

QVariant
ContentModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    if (role == Qt::DisplayRole && orientation == Qt::Horizontal)
        switch (section) {
        case CONTENT_COLUMN_NAME:
            return tr("Name", "heading");
        case CONTENT_COLUMN_TYPE:
            return tr("Type", "heading");
        case CONTENT_COLUMN_SIZE:
            return tr("Size", "heading");
        case CONTENT_COLUMN_REFCOUNT:
            return tr("Refcount", "heading");
        default:
            break;
        }

    return QVariant();
}

QVariant
ContentModel::data(const QModelIndex &index, int role) const
{
    const auto *content = (const TermContent*)index.internalPointer();
    if (m_tracker && content)
        switch (role) {
        case Qt::DisplayRole:
            switch (index.column()) {
            case CONTENT_COLUMN_NAME:
                return content->tu.toString();
            case CONTENT_COLUMN_TYPE:
                return content->isinline ? tr("Inline image") : tr("File");
            case CONTENT_COLUMN_SIZE:
                return content->size;
            case CONTENT_COLUMN_REFCOUNT:
                return content->refcount;
            default:
                break;
            }
            break;
        case Qt::BackgroundRole:
            return content->animation->colorVariant();
        case Qt::UserRole:
            return QVariant::fromValue((void*)content);
        default:
            break;
        }

    return QVariant();
}

Qt::ItemFlags
ContentModel::flags(const QModelIndex &) const
{
    return Qt::ItemIsEnabled|Qt::ItemIsSelectable;
}

//
// View
//
ContentView::ContentView(ContentModel *model, TermInstance *term) :
    m_model(model),
    m_term(term)
{
    QItemSelectionModel *m = selectionModel();
    setModel(m_model);
    delete m;

    setFocusPolicy(Qt::NoFocus);

    setShowGrid(false);
    setSelectionBehavior(QAbstractItemView::SelectRows);
    setSelectionMode(QAbstractItemView::SingleSelection);
    setAlternatingRowColors(true);

    verticalHeader()->setVisible(false);
    horizontalHeader()->setSectionResizeMode(QHeaderView::Interactive);
    horizontalHeader()->setSectionsMovable(true);
}

void
ContentView::contextMenuEvent(QContextMenuEvent *event)
{
    TermContent *content = nullptr;

    QModelIndex index = indexAt(event->pos());
    if (index.isValid()) {
        content = CONTENT_CONTENTP(index);
    }

    auto *manager = g_listener->activeManager();
    if (!content || !manager || !manager->parent()) {
        return;
    }

    TermContentPopup params;
    params.id = content->id;
    params.tu = content->tu;
    params.isimage = content->isinline;
    params.isshown = true;
    params.addTermMenu = false;

    QMenu *menu = manager->parent()->getImagePopup(m_term, &params);
    menu->popup(event->globalPos());
    event->accept();
}

void
ContentView::selectId(const QString &id)
{
    QModelIndex index = m_model->index(id);

    if (index.isValid()) {
        selectRow(index.row());
        scrollTo(index);
    }
}
