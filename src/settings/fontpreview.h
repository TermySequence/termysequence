// Copyright © 2018 TermySequence LLC
//
// SPDX-License-Identifier: GPL-2.0-only

#pragma once

#include "base/fontbase.h"

#include <QWidget>

class FontPreview final: public QWidget, public FontBase
{
private:
    QFont m_font;
    QString m_name;

    QSize m_sizeHint;

protected:
    void paintEvent(QPaintEvent *event);

public:
    FontPreview();

    void setFont(const QFont &font);

    QSize sizeHint() const;
};
