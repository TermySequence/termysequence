// Copyright © 2018 TermySequence LLC
//
// SPDX-License-Identifier: GPL-2.0-only

#pragma once

#include "base/fontbase.h"

#include <QWidget>

class ColorPreview final: public QWidget, public FontBase
{
private:
    QColor m_color;

protected:
    bool event(QEvent *event);
    void paintEvent(QPaintEvent *event);

public:
    ColorPreview();

    inline const QColor& color() const { return m_color; }
    void setColor(const QColor &color);

    QSize sizeHint() const;
};
