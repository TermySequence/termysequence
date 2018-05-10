// Copyright © 2018 TermySequence LLC
//
// SPDX-License-Identifier: GPL-2.0-only

#include "common.h"
#include "thememodel.h"
#include "themeitem.h"
#include "settings.h"
#include "theme.h"

#include <QHeaderView>
#include <QMouseEvent>
#include <algorithm>

#define THEME_COLUMN_SAMPLE      0
#define THEME_COLUMN_NAME        1
#define THEME_N_COLUMNS          2

ThemeModel::ThemeModel(const TermPalette &palette, const TermPalette &saved,
                       const QFont &font, QObject *parent) :
    QAbstractTableModel(parent),
    m_palette(palette),
    m_saved(saved),
    m_font(font)
{
    connect(g_settings, SIGNAL(themesChanged()), SLOT(reloadThemes()));
    reloadThemes();
}

/*
 * Model functions
 */
int
ThemeModel::columnCount(const QModelIndex &parent) const
{
    return THEME_N_COLUMNS;
}

int
ThemeModel::rowCount(const QModelIndex &parent) const
{
    return m_rows;
}

QModelIndex
ThemeModel::index(int row, int column, const QModelIndex &parent) const
{
    if (row >= 0 && row < m_themes.size())
        return createIndex(row, column, (void *)m_themes.at(row));
    else
        return QModelIndex();
}

QVariant
ThemeModel::data(const QModelIndex &index, int role) const
{
    const auto *theme = (const ThemeSettings *)index.internalPointer();
    const auto &content = theme ? theme->content() : m_edits;

    if (theme || (m_edited && index.row() == 0))
        switch (index.column()) {
        case THEME_COLUMN_SAMPLE:
            switch (role) {
            case Qt::DisplayRole:
                return theme ?
                    (theme->builtin() ? tr("Builtin Theme") : tr("Saved Theme")) :
                    tr("Custom Colors");
            case Qt::FontRole:
                return m_font;
            case Qt::TextAlignmentRole:
                return Qt::AlignCenter;
            case Qt::BackgroundRole:
                return QColor(content.bg());
            case Qt::ForegroundRole:
                return QColor(content.fg());
            }
            break;
        case THEME_COLUMN_NAME:
            switch (role) {
            case Qt::UserRole:
                return theme ? theme->name() : tr("Custom Colors");
            case THEME_ROLE_GROUP:
                return theme ? theme->group() : tr("Unsaved changes made in this dialog");
            case THEME_ROLE_ACTIVE:
                return index.row() == m_row;
            }
            break;
        }

    return QVariant();
}

Qt::ItemFlags
ThemeModel::flags(const QModelIndex &index) const
{
    return Qt::NoItemFlags;
}

bool
ThemeModel::calculateRow()
{
    int row = -1;
    bool rc = false;

    if (m_rowHint >= m_edited && m_rowHint < m_rows &&
        m_themes[m_rowHint]->content() == m_palette) {
        row = m_rowHint;
        goto next;
    }

    for (int i = m_edited; i < m_rows; ++i)
        if (m_themes[i]->content() == m_palette) {
            row = i;
            goto next;
        }

    if (m_saved != m_palette) {
        row = 0;
        if (!m_edited || m_edits != m_palette) {
            // We have new edits
            m_edits = m_palette;
            rc = true;
            if (!m_edited) {
                beginInsertRows(QModelIndex(), 0, 0);
                m_edited = true;
                m_themes.prepend(nullptr);
                ++m_rows;
                endInsertRows();
            }
        }
    }

next:
    if (m_row != row) {
        m_row = row;
        rc = true;
    }

    m_rowHint = -1;
    emit rowChanged();
    return rc;
}

void
ThemeModel::reloadData()
{
    if (calculateRow()) {
        QModelIndex start = createIndex(0, THEME_COLUMN_SAMPLE);
        QModelIndex end = createIndex(m_rows - 1, THEME_N_COLUMNS - 1);
        emit dataChanged(start, end);
    }
}

void
ThemeModel::reloadThemes()
{
    beginResetModel();

    for (int i = m_edited; i < m_themes.size(); ++i)
        m_themes[i]->putReference();

    m_themes = g_settings->themes();
    m_rows = m_themes.size();

    for (auto i: qAsConst(m_themes))
        i->takeReference();

    if (m_edited) {
        m_themes.prepend(nullptr);
        ++m_rows;
    }

    endResetModel();
    calculateRow();
}

const TermPalette &
ThemeModel::palette(int row) const
{
    return (row >= m_edited) ? m_themes[row]->content() : m_edits;
}

ThemeSettings *
ThemeModel::currentTheme() const
{
    return (m_row != -1) ? m_themes[m_row] : nullptr;
}

bool
ThemeModel::themeRemovable() const
{
    return (m_row >= m_edited) && !m_themes[m_row]->builtin();
}

//
// View
//
ThemeView::ThemeView(ThemeModel *model)
{
    QItemSelectionModel *m = selectionModel();
    setModel(model);
    delete m;

    setShowGrid(false);
    setSelectionBehavior(QAbstractItemView::SelectRows);
    setSelectionMode(QAbstractItemView::SingleSelection);
    verticalHeader()->setVisible(false);
    horizontalHeader()->setSectionResizeMode(QHeaderView::ResizeToContents);
    horizontalHeader()->setStretchLastSection(true);
    horizontalHeader()->setVisible(false);

    auto *themeSample = new ThemeSampleItemDelegate(this);
    setItemDelegateForColumn(THEME_COLUMN_SAMPLE, themeSample);
    auto *themeName = new ThemeNameItemDelegate(this);
    setItemDelegateForColumn(THEME_COLUMN_NAME, themeName);

    connect(model, &ThemeModel::modelReset, this, &ThemeView::resizeRowsToContents);
    connect(model, &ThemeModel::rowsInserted, this, &ThemeView::resizeRowsToContents);
    resizeRowsToContents();
}

void
ThemeView::mousePressEvent(QMouseEvent *event)
{
    QModelIndex index = indexAt(event->pos());

    if (index.isValid()) {
        emit rowClicked(index.row());
        event->accept();
    } else {
        QTableView::mousePressEvent(event);
    }
}
