// Copyright © 2018 TermySequence LLC
//
// SPDX-License-Identifier: GPL-2.0-only

#include "common.h"
#include "app/selhelper.h"
#include "app/simpleitem.h"
#include "base/thumbicon.h"
#include "imagemodel.h"

#include <QSvgRenderer>
#include <QHeaderView>
#include <QMouseEvent>

#define ICON_N_COLUMNS 6

//
// Model
//
ImageModel::ImageModel(int iconType, QObject *parent) :
    QAbstractTableModel(parent),
    m_type(iconType)
{
    m_all = ThumbIcon::getAllNames((ThumbIcon::IconType)iconType);
    m_cur = m_all;
}

void
ImageModel::setSearchString(const QString &text)
{
    beginResetModel();
    m_cur.clear();
    for (auto &i: qAsConst(m_all))
        if (i.contains(text, Qt::CaseInsensitive))
            m_cur.push_back(i);
    endResetModel();
}

int
ImageModel::indexOf(const QString &name)
{
    int idx = m_cur.indexOf(name);
    if (idx != -1)
        return idx;
    else if (!m_cur.isEmpty())
        return 0;
    else
        return -1;
}

/*
 * Model functions
 */
int
ImageModel::columnCount(const QModelIndex &) const
{
    return ICON_N_COLUMNS;
}

int
ImageModel::rowCount(const QModelIndex &) const
{
    return (m_cur.size() + ICON_N_COLUMNS - 1) / ICON_N_COLUMNS;
}

QVariant
ImageModel::data(const QModelIndex &index, int role) const
{
    int pos = index.row() * ICON_N_COLUMNS + index.column();
    if (pos < m_cur.size()) {
        const auto &name = m_cur.at(pos);
        QSvgRenderer *ptr;

        switch (role) {
        case Qt::DisplayRole:
            return name;
        case Qt::TextAlignmentRole:
            return (int)(Qt::AlignHCenter|Qt::AlignBottom);
        case Qt::UserRole:
            ptr = ThumbIcon::getRenderer((ThumbIcon::IconType)m_type, name);
            return QVariant::fromValue(ptr);
        default:
            break;
        }
    }
    return QVariant();
}

Qt::ItemFlags
ImageModel::flags(const QModelIndex &index) const
{
    int pos = index.row() * ICON_N_COLUMNS + index.column();
    if (pos < m_cur.size())
        return Qt::ItemIsEnabled|Qt::ItemIsSelectable;
    else
        return 0;
}

//
// View
//
ImageView::ImageView(ImageModel *model) :
    m_model(model)
{
    QItemSelectionModel *m = selectionModel();
    setModel(model);
    delete m;

    setShowGrid(false);
    setSelectionBehavior(QAbstractItemView::SelectItems);
    setSelectionMode(QAbstractItemView::SingleSelection);
    setItemDelegate(new SvgImageItemDelegate(this));
    setWordWrap(false);
    verticalHeader()->setVisible(false);
    horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    horizontalHeader()->hide();
}

void
ImageView::mouseDoubleClickEvent(QMouseEvent *event)
{
    if (event->button() == Qt::LeftButton) {
        emit doubleClicked();
    }

    event->accept();
}

QString
ImageView::selectedImage() const
{
    QModelIndexList sel = selectionModel()->selectedIndexes();
    return !sel.isEmpty() ? sel.at(0).data().toString() : QString();
}

void
ImageView::clearSelectedImage()
{
    selectionModel()->clearSelection();
}

void
ImageView::setSelectedImage(const QString &cur)
{
    int pos = m_model->indexOf(cur);
    if (pos != -1) {
        int row = pos / ICON_N_COLUMNS, col = pos % ICON_N_COLUMNS;
        QModelIndex index = m_model->index(row, col);
        doSelectIndex(this, index, false);
    }
}
