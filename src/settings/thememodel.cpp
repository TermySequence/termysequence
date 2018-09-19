// Copyright © 2018 TermySequence LLC
//
// SPDX-License-Identifier: GPL-2.0-only

#include "common.h"
#include "app/attrbase.h"
#include "thememodel.h"
#include "themeitem.h"
#include "settings.h"
#include "theme.h"

#include <QHeaderView>
#include <QMouseEvent>

#define THEME_COLUMN_SAMPLE      0
#define THEME_COLUMN_NAME        1
#define THEME_N_COLUMNS          2

#define TR_NAME1 TL("settings-name", "Custom Color Theme")

ThemeModel::ThemeModel(const TermPalette &palette, const TermPalette &saved,
                       const QFont &font, QObject *parent) :
    QAbstractTableModel(parent),
    m_palette(palette),
    m_saved(saved),
    m_edits(saved),
    m_font(font)
{
    // Note: reloadThemes called from ThemeTab
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
    return m_themes.size();
}

QModelIndex
ThemeModel::index(int row, int column, const QModelIndex &parent) const
{
    if (row >= 0 && row < m_themes.size())
        return createIndex(row, column);
    else
        return QModelIndex();
}

QVariant
ThemeModel::data(const QModelIndex &index, int role) const
{
    ThemeType type = m_types[index.row()];
    const auto *theme = m_themes[index.row()];

    if (index.column() == THEME_COLUMN_SAMPLE) {
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
            switch (type) {
            case NoTheme:
                return QColor(m_saved.bg());
            case Unsaved:
                return QColor(m_edits.bg());
            default:
                return QColor(theme->content().bg());
            }
        case Qt::ForegroundRole:
            switch (type) {
            case NoTheme:
                return QColor(m_saved.fg());
            case Unsaved:
                return QColor(m_edits.fg());
            default:
                return QColor(theme->content().fg());
            }
        default:
            break;
        }
    }

    switch (role) {
    case THEME_ROLE_NAME:
        switch (type) {
        case NoTheme:
            return TR_NAME1;
        case Unsaved:
            return tr("Unsaved Color Theme");
        default:
            return theme->name();
        }
    case THEME_ROLE_GROUP:
        switch (type) {
        case NoTheme:
            return tr("Custom colors not matching any saved theme");
        case Unsaved:
            return tr("Unsaved changes made in this dialog");
        default:
            return theme->group();
        }
    case THEME_ROLE_THEME:
        return QVariant::fromValue((void*)theme);
    case THEME_ROLE_PALETTE:
        switch (type) {
        case NoTheme:
            return QVariant::fromValue((void*)&m_saved);
        case Unsaved:
            return QVariant::fromValue((void*)&m_edits);
        default:
            return QVariant::fromValue((void*)&theme->content());
        }
    case THEME_ROLE_REMOVABLE:
        return theme ? !theme->builtin() : false;
    default:
        break;
    }

    return QVariant();
}

Qt::ItemFlags
ThemeModel::flags(const QModelIndex &index) const
{
    return index.column() == THEME_COLUMN_NAME ?
        Qt::ItemIsSelectable|Qt::ItemIsEnabled :
        Qt::NoItemFlags;
}

inline void
ThemeModel::startEditing()
{
    if (!m_edited || m_edits != m_palette) {
        // We have new edits
        m_edits = m_palette;
        if (!m_edited) {
            beginInsertRows(QModelIndex(), 0, 0);
            m_themes.prepend(nullptr);
            m_types.prepend(Unsaved);
            m_edited = true;
            ++m_special;
            endInsertRows();
        }
    }
}

inline void
ThemeModel::stopEditing()
{
    if (m_edited) {
        beginRemoveRows(QModelIndex(), 0, 0);
        m_themes.pop_front();
        m_types.pop_front();
        m_edited = false;
        --m_special;
        endRemoveRows();
    }
}

int
ThemeModel::reloadData()
{
    for (int i = m_special, n = m_themes.size(); i < n; ++i) {
        if (m_themes[i]->content() == m_palette) {
            int row = i - m_edited;
            stopEditing();
            return row;
        }
    }

    if (m_saved == m_palette) {
        stopEditing();
    } else {
        startEditing();
    }

    return 0;
}

int
ThemeModel::reloadThemes()
{
    beginResetModel();

    for (int i = m_special, n = m_themes.size(); i < n; ++i)
        m_themes[i]->putReference();

    m_themes = g_settings->themes();
    m_types.fill(YesTheme, m_themes.size());

    bool custom = true, edited = m_edited;
    int row = -1;

    for (int i = 0, n = m_themes.size(); i < n; ++i) {
        auto *theme = m_themes[i];
        theme->takeReference();
        if (theme->content() == m_palette)
            row = i;
        if (theme->content() == m_saved)
            custom = false;
        if (theme->content() == m_edits)
            edited = false;
    }
    if ((m_special = custom)) {
        m_themes.prepend(nullptr);
        m_types.prepend(NoTheme);
    }
    if ((m_edited = edited && m_edits != m_saved)) {
        m_themes.prepend(nullptr);
        m_types.prepend(Unsaved);
        ++m_special;
    }

    if (row == -1) {
        row = (m_palette == m_saved) ? m_edited : 0;
    } else {
        row += m_special;
    }

    endResetModel();
    return row;
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

ThemeSettings *
ThemeView::currentTheme() const
{
    auto indexes = selectionModel()->selectedIndexes();

    return !indexes.isEmpty() ?
        THEME_THEMEP(indexes.at(0)) :
        nullptr;
}
