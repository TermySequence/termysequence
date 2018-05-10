// Copyright © 2018 TermySequence LLC
//
// SPDX-License-Identifier: GPL-2.0-only

#pragma once

#include "palette.h"

#include <QDialog>

QT_BEGIN_NAMESPACE
class QTabWidget;
QT_END_NAMESPACE
class ThemeTab;
class PaletteTab;
class ColorsTab;
class DircolorsTab;

class PaletteDialog final: public QDialog
{
    Q_OBJECT

private:
    TermPalette m_palette;
    TermPalette m_saved;

    ThemeTab *m_preTab;
    PaletteTab *m_stdTab;
    ColorsTab *m_xtdTab;
    DircolorsTab *m_dirTab;
    QTabWidget *m_tabs;

    void reloadTermcolors(QObject *sender = nullptr);
    void reloadDircolors(QObject *sender = nullptr);

private slots:
    void reportTermcolorsModified();
    void reportDircolorsModified();

    void handleReset();

signals:
    void termcolorsModified();
    void dircolorsModified();

public:
    PaletteDialog(const TermPalette &palette, const QFont &font, QWidget *parent);

    inline const TermPalette& saved() const { return m_saved; }
    inline const TermPalette& palette() const { return m_palette; }

    QSize sizeHint() const { return QSize(880, 880); }

    void bringUp();
};
