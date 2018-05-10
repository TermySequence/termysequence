// Copyright © 2018 TermySequence LLC
//
// SPDX-License-Identifier: GPL-2.0-only

#pragma once

#include "app/flags.h"
#include "lib/palette.h"

#include <QRgb>
#include <QVector>
#include <QString>

class Termcolors: public QVector<QRgb>
{
private:
    void setDefaults();

public:
    Termcolors();
    Termcolors(const QString &spec);
    void parse(const QString &spec);

    inline QRgb std(unsigned i) const { return at(i & PALETTE_STANDARD_MASK); }

    inline QRgb bg() const { return at(PALETTE_APP_BG); }
    inline void setBg(QRgb bg) { replace(PALETTE_APP_BG, bg & PALETTE_COLOR); }
    inline QRgb fg() const { return at(PALETTE_APP_FG); }
    inline void setFg(QRgb fg) { replace(PALETTE_APP_FG, fg & PALETTE_COLOR); }

    QRgb specialBg(Tsqt::CellFlags flags, QRgb fallback) const;
    QRgb specialFg(Tsqt::CellFlags flags, QRgb fallback) const;

    QString tStr() const;
};
