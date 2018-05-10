// Copyright © 2018 TermySequence LLC
//
// SPDX-License-Identifier: GPL-2.0-only

#pragma once

#include "palette.h"

#include <QDialog>

class DircolorsTab;

class DircolorsDialog final: public QDialog
{
    Q_OBJECT

private:
    TermPalette m_palette;
    Dircolors m_saved;

    DircolorsTab *m_arrange;

private slots:
    void handleReset();

signals:
    void modified();

public:
    DircolorsDialog(const TermPalette &palette, const QFont &font,
                    QWidget *parent);

    inline const Dircolors& saved() const { return m_saved; }
    inline const Dircolors& dircolors() const { return m_palette; }

    QSize sizeHint() const { return QSize(800, 640); }

    void bringUp();
};
