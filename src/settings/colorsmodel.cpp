// Copyright © 2018 TermySequence LLC
//
// SPDX-License-Identifier: GPL-2.0-only

#include "common.h"
#include "app/attrbase.h"
#include "app/simpleitem.h"
#include "colorsmodel.h"

#include <QHeaderView>
#include <QFontDatabase>

ColorsModel::ColorsModel(const Termcolors &tcpal, QObject *parent) :
    QAbstractTableModel(parent),
    m_tcpal(tcpal),
    m_font(QFontDatabase::systemFont(QFontDatabase::FixedFont)),
    m_offset(PALETTE_APP + 4),
    m_rows(PALETTE_APP_SIZE - 12)
{
}

int
ColorsModel::row2pos(int row) const
{
    return m_offset + row + (row > 1) * 8;
}

int
ColorsModel::pos2row(int pos) const
{
    pos -= m_offset;
    if (pos < 2)
        return pos;
    else if (pos < 11)
        return 2;
    else
        return pos - 8;
}

void
ColorsModel::reloadData()
{
    QModelIndex start = createIndex(0, COLORS_COLUMN_COLOR);
    QModelIndex end = createIndex(m_rows - 1, COLORS_COLUMN_TEXT);
    emit dataChanged(start, end);
}

void
ColorsModel::reloadOne(int idx)
{
    int row = pos2row(idx);
    QModelIndex start = createIndex(row, COLORS_COLUMN_COLOR);
    QModelIndex end = createIndex(row, COLORS_COLUMN_TEXT);
    emit dataChanged(start, end);
}

/*
 * Model functions
 */
int
ColorsModel::columnCount(const QModelIndex &parent) const
{
    return COLORS_N_COLUMNS;
}

int
ColorsModel::rowCount(const QModelIndex &parent) const
{
    return m_rows;
}

QVariant
ColorsModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    if (role == Qt::DisplayRole) {
        if (orientation == Qt::Horizontal) {
            switch (section) {
            case COLORS_COLUMN_DESCRIPTION:
                return tr("Description", "heading");
            case COLORS_COLUMN_COLOR:
                return tr("Color Display", "heading");
            case COLORS_COLUMN_TEXT:
                return tr("Value", "heading");
            default:
                break;
            }
        } else {
            return QString::number(row2pos(section));
        }
    }

    return QVariant();
}

static const char *s_colorDesc[] = {
    TN("color-name", "Terminal foreground color"),
    TN("color-name", "Terminal background color"),
    TN("color-name", "Inactive selection handle"),
    TN("color-name", "Active selection handle"),
    TN("color-name", "User annotation background"),
    TN("color-name", "User annotation foreground"),
    TN("color-name", "Search match text background"),
    TN("color-name", "Search match text foreground"),
    TN("color-name", "Search match line background"),
    TN("color-name", "Search match line foreground"),
    TN("color-name", "Selected prompt text background"),
    TN("color-name", "Selected prompt text foreground"),
    TN("color-name", "Prompt text background"),
    TN("color-name", "Prompt text foreground"),
    TN("color-name", "Command text background"),
    TN("color-name", "Command text foreground"),
    TN("color-name", "Mark selected prompt background"),
    TN("color-name", "Mark selected prompt foreground"),
    TN("color-name", "Mark current prompt background"),
    TN("color-name", "Mark current prompt foreground"),
    TN("color-name", "Mark running background"),
    TN("color-name", "Mark running foreground"),
    TN("color-name", "Mark exit status zero background"),
    TN("color-name", "Mark exit status zero foreground"),
    TN("color-name", "Mark exit status nonzero background"),
    TN("color-name", "Mark exit status nonzero foreground"),
    TN("color-name", "Mark exit on signal background"),
    TN("color-name", "Mark exit on signal foreground"),
    TN("color-name", "Mark user annotation background"),
    TN("color-name", "Mark user annotation foreground"),
    TN("color-name", "Mark search match background"),
    TN("color-name", "Mark search match foreground"),
};

QVariant
ColorsModel::colorText(QRgb value) const
{
    if (PALETTE_IS_DISABLED(value))
        return tr("Color is disabled");
    else
        return QColor(value).name(QColor::HexRgb);
}

QVariant
ColorsModel::data(const QModelIndex &index, int role) const
{
    int row = index.row();
    int pos = row2pos(row);

    switch (index.column()) {
    case COLORS_COLUMN_DESCRIPTION:
        switch (role) {
        case Qt::DisplayRole:
            return QCoreApplication::translate("color-name", s_colorDesc[row]);
        case Qt::FontRole:
            // Highlight certain rows in bold
            if (row < 2) {
                QFont tmp;
                tmp.setBold(true);
                return tmp;
            }
            break;
        }
        break;
    case COLORS_COLUMN_COLOR:
        switch (role) {
        case Qt::UserRole:
            return m_tcpal[pos];
        case Qt::BackgroundRole:
            return QColor(m_tcpal[pos]);
        }
        break;
    case COLORS_COLUMN_TEXT:
        if (role == Qt::DisplayRole)
            return colorText(m_tcpal[pos]);
        else if (role == Qt::FontRole && PALETTE_IS_ENABLED(m_tcpal[pos]))
            return m_font;
        break;
    }

    return QVariant();
}

Qt::ItemFlags
ColorsModel::flags(const QModelIndex &index) const
{
    return Qt::ItemIsEnabled|Qt::ItemIsSelectable;
}

//
// View
//
ColorsView::ColorsView(ColorsModel *model)
{
    QItemSelectionModel *m = selectionModel();
    setModel(model);
    delete m;

    setShowGrid(false);
    setSelectionBehavior(QAbstractItemView::SelectRows);
    setSelectionMode(QAbstractItemView::SingleSelection);

    horizontalHeader()->setSectionResizeMode(QHeaderView::ResizeToContents);
    horizontalHeader()->setDefaultAlignment(Qt::AlignLeft|Qt::AlignVCenter);

    ColorItemDelegate *colorItem = new ColorItemDelegate(this);
    setItemDelegateForColumn(COLORS_COLUMN_COLOR, colorItem);
}
