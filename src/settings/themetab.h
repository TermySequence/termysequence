// Copyright © 2018 TermySequence LLC
//
// SPDX-License-Identifier: GPL-2.0-only

#pragma once

#include "base/fontbase.h"
#include "palette.h"

#include <QWidget>

QT_BEGIN_NAMESPACE
class QPushButton;
QT_END_NAMESPACE
class ThemeModel;
class ThemeView;
class ThemeSettings;
class NewThemeDialog;

//
// Preview Widget
//
class ThemePreview final: public QWidget, public FontBase
{
private:
    const TermPalette &m_palette;

    QFont m_font;
    QSize m_sizeHint;

protected:
    void paintEvent(QPaintEvent *event);

public:
    ThemePreview(const TermPalette &palette, const QFont &font);

    QSize sizeHint() const;
};

//
// Tab Widget
//
class ThemeTab final: public QWidget
{
    Q_OBJECT

private:
    TermPalette &m_palette;

    ThemeModel *m_model;
    ThemeView *m_view;
    ThemePreview *m_preview;

    QPushButton *m_renameButton;
    QPushButton *m_deleteButton;

    NewThemeDialog *m_dialog;

    QMetaObject::Connection m_mocSel;

    ThemeSettings* doSave();
    ThemeSettings* doRename();

private slots:
    void handleSelection();
    void handleThemes();

    void handleSave();
    void handleSaveAnswer();
    void handleSaveConfirm(int result);
    void handleRename();
    void handleDelete();
    void handleReload();

signals:
    void modified();

public:
    ThemeTab(TermPalette &palette, const TermPalette &saved, const QFont &font);

    void reload();
    void takeFocus();
};
