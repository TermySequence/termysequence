// Copyright © 2018 TermySequence LLC
//
// SPDX-License-Identifier: GPL-2.0-only

#include "common.h"
#include "palette.h"
#include "lib/defcolors.h"

//
// Main Palette
//
inline void
Termcolors::setDefaults()
{
    if (isEmpty()) {
        for (int i = 0; i < PALETTE_SIZE; ++i)
            append(s_defaultPalette[i]);
    } else {
        for (int i = 0; i < PALETTE_SIZE; ++i)
            replace(i, s_defaultPalette[i]);
    }
}

void
Termcolors::parse(const QString &spec)
{
    setDefaults();

    // list consists of (index,value) pairs
    QStringList list = spec.split(',');

    if (list.size() % 2)
        list.pop_back();

    for (int i = 0; i < list.size(); i += 2) {
        int idx = list.at(i).toInt(NULL, 16);
        QRgb value = list.at(i + 1).toUInt(NULL, 16);

        if (idx >= 0 && idx < PALETTE_SIZE)
            replace(idx, value & PALETTE_VALUEMASK);
    }
}

Termcolors::Termcolors()
{
    setDefaults();
}

Termcolors::Termcolors(const QString &spec)
{
    parse(spec);
}

QRgb
Termcolors::specialBg(Tsqt::CellFlags flags, QRgb fallback) const
{
    QRgb rc;

    if (flags & Tsqt::SearchText)
        rc = QVector::at(PALETTE_SH_MATCHTEXT_BG);
    else if (flags & Tsqt::Annotation)
        rc = QVector::at(PALETTE_SH_NOTE_BG);
    else if (flags & Tsqt::SearchLine)
        rc = QVector::at(PALETTE_SH_MATCHLINE_BG);
    else if (flags & Tsqt::ActivePrompt)
        rc = QVector::at(PALETTE_SH_SELECTED_BG);
    else if (flags & Tsq::Prompt)
        rc = flags & Tsq::Bg ? PALETTE_DISABLED : QVector::at(PALETTE_SH_PROMPT_BG);
    else // (flags & Tsq::Command)
        rc = flags & Tsq::Bg ? PALETTE_DISABLED : QVector::at(PALETTE_SH_COMMAND_BG);

    return PALETTE_IS_DISABLED(rc) ? fallback : rc;
}

QRgb
Termcolors::specialFg(Tsqt::CellFlags flags, QRgb fallback) const
{
    QRgb rc;

    if (flags & Tsqt::SearchText)
        rc = QVector::at(PALETTE_SH_MATCHTEXT_FG);
    else if (flags & Tsqt::Annotation)
        rc = QVector::at(PALETTE_SH_NOTE_FG);
    else if (flags & Tsqt::SearchLine)
        rc = QVector::at(PALETTE_SH_MATCHLINE_FG);
    else if (flags & Tsqt::ActivePrompt)
        rc = QVector::at(PALETTE_SH_SELECTED_FG);
    else if (flags & Tsq::Prompt)
        rc = flags & Tsq::Fg ? PALETTE_DISABLED : QVector::at(PALETTE_SH_PROMPT_FG);
    else // (flags & Tsq::Command)
        rc = flags & Tsq::Fg ? PALETTE_DISABLED : QVector::at(PALETTE_SH_COMMAND_FG);

    return PALETTE_IS_DISABLED(rc) ? fallback : rc;
}

QString
Termcolors::tStr() const
{
    QStringList result;

    for (int i = 0; i < PALETTE_SIZE; ++i) {
        if (QVector::at(i) != s_defaultPalette[i]) {
            result.append(QString::number(i, 16));
            result.append(QString::number(QVector::at(i), 16));
        }
    }

    return result.join(',');
}

//
// Amalgamated Object
//
TermPalette::TermPalette(const QString &tc, const QString &dc) :
    Termcolors(tc), Dircolors(dc)
{
}
