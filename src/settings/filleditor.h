// Copyright © 2018 TermySequence LLC
//
// SPDX-License-Identifier: GPL-2.0-only

#pragma once

#include <QDialog>

#include "termlayout.h"
#include "termcolors.h"

QT_BEGIN_NAMESPACE
class QSpinBox;
QT_END_NAMESPACE

//
// Preview
//
class FillPreview final: public QWidget
{
    Q_OBJECT

private:
    const Termcolors &m_tcpal;
    int m_index = -1;
    int m_height;
    int m_line;

protected:
    void paintEvent(QPaintEvent *event);

public:
    FillPreview(const Termcolors &tcpal, const QFont &font);

    QSize sizeHint() const;

public slots:
    void setColorIndex(int index);
};

//
// Dialog
//
class FillEditor final: public QDialog
{
private:
    QSpinBox *m_column, *m_color;
    FillPreview *m_preview;

public:
    FillEditor(QWidget *parent, const Termcolors &tcpal,
               const QFont &font, bool adding);

    TermLayout::Fill fill() const;
    void setFill(const TermLayout::Fill &fill);
};
