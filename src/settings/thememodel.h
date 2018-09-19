// Copyright © 2018 TermySequence LLC
//
// SPDX-License-Identifier: GPL-2.0-only

#pragma once

#include "palette.h"

#include <QAbstractTableModel>
#include <QTableView>

class ThemeSettings;

#define THEME_ROLE_NAME         Qt::ItemDataRole(Qt::UserRole)
#define THEME_ROLE_GROUP        Qt::ItemDataRole(Qt::UserRole + 1)
#define THEME_ROLE_THEME        Qt::ItemDataRole(Qt::UserRole + 2)
#define THEME_ROLE_PALETTE      Qt::ItemDataRole(Qt::UserRole + 3)
#define THEME_ROLE_REMOVABLE    Qt::ItemDataRole(Qt::UserRole + 4)

#define THEME_THEMEP(i) \
    static_cast<ThemeSettings*>(i.data(THEME_ROLE_THEME).value<void*>())
#define THEME_PALETTEP(i) \
    static_cast<const TermPalette*>(i.data(THEME_ROLE_PALETTE).value<void*>())

//
// Model
//
class ThemeModel final: public QAbstractTableModel
{
    Q_OBJECT

private:
    enum ThemeType { YesTheme, NoTheme, Unsaved };

    const TermPalette &m_palette;
    const TermPalette &m_saved;
    TermPalette m_edits;

    QVector<ThemeSettings*> m_themes;
    QVector<ThemeType> m_types;

    bool m_edited = false;
    int m_special = 0;

    QFont m_font;

    void startEditing();
    void stopEditing();

public:
    ThemeModel(const TermPalette &palette, const TermPalette &saved,
               const QFont &font, QObject *parent);

    int columnCount(const QModelIndex &parent) const;
    int rowCount(const QModelIndex &parent = QModelIndex()) const;
    QModelIndex index(int row, int column, const QModelIndex &parent = QModelIndex()) const;

    QVariant data(const QModelIndex &index, int role) const;
    Qt::ItemFlags flags(const QModelIndex &index) const;

    int reloadData();
    int reloadThemes();
};

//
// View
//
class ThemeView final: public QTableView
{
public:
    ThemeView(ThemeModel *model);

    ThemeSettings* currentTheme() const;
};
