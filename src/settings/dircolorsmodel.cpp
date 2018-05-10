// Copyright © 2018 TermySequence LLC
//
// SPDX-License-Identifier: GPL-2.0-only

#include "common.h"
#include "app/attrbase.h"
#include "dircolorsmodel.h"

#include <QHeaderView>
#include <QMouseEvent>

#define DIRCOLORS_COLUMN_INHERIT    0
#define DIRCOLORS_COLUMN_TYPE       1
#define DIRCOLORS_COLUMN_NAME       2
#define DIRCOLORS_COLUMN_VALUE      3
#define DIRCOLORS_COLUMN_PREVIEW    4
#define DIRCOLORS_N_COLUMNS         5

#define DIRCOLORS_ENTP(i) static_cast<const Dircolors::Entry*>(i.internalPointer())

DircolorsModel::DircolorsModel(const TermPalette &palette, const QFont &font,
                               QObject *parent) :
    QAbstractTableModel(parent),
    m_palette(palette),
    m_font(font)
{
    m_sample = QString(tr("Sample") + A(".txt  ")).repeated(3);
    m_sample.chop(2);
    m_blink = QString(tr("Blinking") + A(".txt  ")).repeated(3);
    m_blink.chop(2);
}

void
DircolorsModel::reloadData()
{
    beginResetModel();
    m_dircolors.parseForDisplay(m_palette.dStr(), m_list);
    endResetModel();

#if 0
    for (auto &ent: m_list) {
        fprintf(stderr, "{ A(\"%s\"), A(\"%s\"), %u, { 0x%x, %u, %u } },\n",
                qUtf8Printable(ent.id), qUtf8Printable(ent.spec), ent.category,
                (unsigned)ent.attr.flags, ent.attr.fg, ent.attr.bg);
    }
#endif
}

/*
 * Model functions
 */
int
DircolorsModel::columnCount(const QModelIndex &parent) const
{
    return DIRCOLORS_N_COLUMNS;
}

int
DircolorsModel::rowCount(const QModelIndex &parent) const
{
    return m_list.size();
}

QModelIndex
DircolorsModel::index(int row, int column, const QModelIndex &) const
{
    if (row < m_list.size())
        return createIndex(row, column, (void *)&m_list[row]);
    else
        return QModelIndex();
}

QVariant
DircolorsModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    if (orientation == Qt::Horizontal && role == Qt::DisplayRole)
        switch (section) {
        case DIRCOLORS_COLUMN_INHERIT:
            return tr("Inherited", "heading");
        case DIRCOLORS_COLUMN_TYPE:
            return tr("Type", "heading");
        case DIRCOLORS_COLUMN_NAME:
            return tr("Name", "heading");
        case DIRCOLORS_COLUMN_VALUE:
            return tr("Value", "heading");
        case DIRCOLORS_COLUMN_PREVIEW:
            return tr("Preview", "heading");
        }

    return QVariant();
}

QVariant
DircolorsModel::data(const QModelIndex &index, int role) const
{
    const auto *ent = (const Dircolors::Entry *)index.internalPointer();
    if (ent)
        switch (role) {
        case Qt::DisplayRole:
            switch (index.column()) {
            case DIRCOLORS_COLUMN_INHERIT:
                if (ent->start == -1)
                    return tr("Yes");
                break;
            case DIRCOLORS_COLUMN_TYPE:
                switch (ent->type) {
                case 1:
                    return tr("Category");
                case 2:
                    return tr("Placeholder");
                default:
                    return tr("Extension");
                }
            case DIRCOLORS_COLUMN_NAME:
                return ent->id;
            case DIRCOLORS_COLUMN_VALUE:
                return ent->spec;
            case DIRCOLORS_COLUMN_PREVIEW:
                return (ent->attr.flags & Tsq::Blink) ? m_blink : m_sample;
            }
            break;
        case Qt::FontRole:
            if (index.column() == DIRCOLORS_COLUMN_PREVIEW) {
                QFont font(m_font);
                font.setBold(ent->attr.flags & Tsq::Bold);
                font.setUnderline(ent->attr.flags & Tsq::Underline);
                return font;
            }
            break;
        case Qt::BackgroundRole:
        case Qt::ForegroundRole:
            if (index.column() == DIRCOLORS_COLUMN_PREVIEW) {
                unsigned fg, bg;
                fg = (ent->attr.flags & Tsq::Fg) ? m_palette.at(ent->attr.fg) : m_palette.fg();
                bg = (ent->attr.flags & Tsq::Bg) ? m_palette.at(ent->attr.bg) : m_palette.bg();
                bool inv = ent->attr.flags & Tsq::Inverse;
                return QColor(inv ^ (role == Qt::ForegroundRole) ? fg : bg);
            }
            break;
        }

    return QVariant();
}

Qt::ItemFlags
DircolorsModel::flags(const QModelIndex &index) const
{
    switch (index.column()) {
    case DIRCOLORS_COLUMN_PREVIEW:
        return Qt::ItemIsEnabled;
    default:
        return Qt::ItemIsSelectable | Qt::ItemIsEnabled;
    }
}

//
// View
//
DircolorsView::DircolorsView(const TermPalette &palette, const QFont &font)
{
    m_model = new DircolorsModel(palette, font, this);

    QItemSelectionModel *m = selectionModel();
    setModel(m_model);
    delete m;

    // setShowGrid(false);
    setSelectionBehavior(QAbstractItemView::SelectRows);
    setSelectionMode(QAbstractItemView::SingleSelection);
    verticalHeader()->setVisible(false);
    horizontalHeader()->setStretchLastSection(true);
    horizontalHeader()->setSectionResizeMode(QHeaderView::ResizeToContents);
}

void
DircolorsView::mousePressEvent(QMouseEvent *event)
{
    if (event->button() == Qt::LeftButton) {
        auto *ent = DIRCOLORS_ENTP(indexAt(event->pos()));
        if (ent && ent->start != -1) {
            emit selectedSubstring(ent->start, ent->end);
        }
    }

    QTableView::mousePressEvent(event);
}

void
DircolorsView::mouseDoubleClickEvent(QMouseEvent *event)
{
    if (event->button() == Qt::LeftButton) {
        auto *ent = DIRCOLORS_ENTP(indexAt(event->pos()));
        if (ent) {
            QString text;
            switch (ent->type) {
            case 0:
                text = A("*.");
                break;
            case 2:
                text = '$';
                break;
            }
            text += ent->id + '=' + ent->spec + ':';
            emit appendedSubstring(text);
        }
    }

    event->accept();
}
