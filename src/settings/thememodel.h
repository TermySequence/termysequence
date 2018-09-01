// Copyright © 2018 TermySequence LLC
//
// SPDX-License-Identifier: GPL-2.0-only

#pragma once

#include "palette.h"

#include <QAbstractTableModel>
#include <QTableView>

class ThemeSettings;

#define THEME_ROLE_GROUP    Qt::ItemDataRole(Qt::UserRole + 1)
#define THEME_ROLE_ACTIVE   Qt::ItemDataRole(Qt::UserRole + 2)

//
// Model
//
class ThemeModel final: public QAbstractTableModel
{
    Q_OBJECT

private:
    const TermPalette &m_palette;
    const TermPalette &m_saved;
    QVector<ThemeSettings*> m_themes;

    TermPalette m_edits;
    bool m_edited = false;

    QFont m_font;

    int m_rows = 0;
    int m_row = -1;
    int m_rowHint = -1;

    bool calculateRow();

private slots:
    void reloadThemes();

signals:
    void rowChanged(int row);

public:
    ThemeModel(const TermPalette &palette, const TermPalette &saved,
               const QFont &font, QObject *parent);

    inline int indexOf(ThemeSettings *theme) const { return m_themes.indexOf(theme); }

    int columnCount(const QModelIndex &parent) const;
    int rowCount(const QModelIndex &parent = QModelIndex()) const;
    QModelIndex index(int row, int column, const QModelIndex &parent = QModelIndex()) const;

    QVariant data(const QModelIndex &index, int role) const;
    Qt::ItemFlags flags(const QModelIndex &index) const;

    const TermPalette& palette(int row) const;
    ThemeSettings* currentTheme() const;
    bool themeRemovable() const;

    inline void setRowHint(int rowHint) { m_rowHint = rowHint; }

    void reloadData();
};

//
// View
//
class ThemeView final: public QTableView
{
    Q_OBJECT

private slots:
    void handleRowChanged(int row);

protected:
    void mousePressEvent(QMouseEvent *event);

signals:
    void rowClicked(int row);

public:
    ThemeView(ThemeModel *model);
};
