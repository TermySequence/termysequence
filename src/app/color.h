// Copyright © 2018 TermySequence LLC
//
// SPDX-License-Identifier: GPL-2.0-only

#pragma once

#include <QColor>

enum ColorName {
    MinorBg, MinorFg, MajorBg, MajorFg,
    StartFg, ErrorFg, FinishFg, CancelFg,
    ConnFg, DisconnFg, BellFg,
    NColors
};

// Color manipulation routines
namespace Colors
{
    // Blending functions
    inline QRgb blend1(QRgb bg, QRgb fg)
    {
        int r = (4 * qRed(bg) + qRed(fg)) / 5;
        int g = (4 * qGreen(bg) + qGreen(fg)) / 5;
        int b = (4 * qBlue(bg) + qBlue(fg)) / 5;
        return qRgb(r, g, b);
    }
    inline QRgb blend1(const QColor &bg, const QColor &fg)
    {
        return blend1(bg.rgb(), fg.rgb());
    }

    inline QRgb blend2(QRgb bg, QRgb fg)
    {
        int r = (3 * qRed(bg) + 2 * qRed(fg)) / 5;
        int g = (3 * qGreen(bg) + 2 * qGreen(fg)) / 5;
        int b = (3 * qBlue(bg) + 2 * qBlue(fg)) / 5;
        return qRgb(r, g, b);
    }
    inline QRgb blend2(const QColor &bg, const QColor &fg)
    {
        return blend2(bg.rgb(), fg.rgb());
    }

    // Blend with alpha
    inline QRgb blend5a(QRgb bg, QRgb fg, int alpha)
    {
        int r = (200 * qRed(bg) + alpha * qRed(fg)) / (200 + alpha);
        int g = (200 * qGreen(bg) + alpha * qGreen(fg)) / (200 + alpha);
        int b = (200 * qBlue(bg) + alpha * qBlue(fg)) / (200 + alpha);
        return qRgb(r, g, b);
    }

    inline QRgb blend0a(QRgb bg, QRgb fg, int alpha)
    {
        int r = (55 * qRed(bg) + alpha * qRed(fg)) / (55 + alpha);
        int g = (55 * qGreen(bg) + alpha * qGreen(fg)) / (55 + alpha);
        int b = (55 * qBlue(bg) + alpha * qBlue(fg)) / (55 + alpha);
        return qRgb(r, g, b);
    }

    // Adjustment functions
    inline QColor whiten(QColor color)
    {
        int h, s, v;
        color.getHsv(&h, &s, &v);
        color.setHsv(h, s / 2, v);
        return color.toRgb();
    }
}
