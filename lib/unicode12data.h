// Copyright © 2019 TermySequence LLC
//
// SPDX-License-Identifier: GPL-2.0-only

#pragma once

#include "unitable.h"

//
// Codepoint types for grapheme-cluster-break algorithm
//
enum GcbParserFlag : uint32_t {
    GcbPictographicSequence    = 1u,
    GcbStateMask               = 255u,

    GcbZwj                     = 1u << 8,
    GcbSkinToneModifier        = 1u << 9,
    GcbHangulL                 = 1u << 10,
    GcbHangulV                 = 1u << 11,
    GcbHangulT                 = 1u << 12,
    GcbHangulLV                = 1u << 13,
    GcbHangulLVT               = 1u << 14,
    GcbFlagsMask               = 255u << 8,

    GcbCombining               = 1u << 16,
    GcbPictographic            = 1u << 17,
    GcbEmojiModifier           = 1u << 18,
    GcbTextModifier            = 1u << 19,
    GcbRegionalIndicator       = 1u << 20,
    GcbHangul                  = 1u << 21,
    GcbInvalid                 = 1u << 22,
    GcbBaseMask                = 255u << 16,

    // Values from Tsq::CellFlags
    EmojiChar                  = 1u << 30,
    DblWidthChar               = 1u << 31,
    PerCharFlags               = 3u << 30,

    GcbPictographicJoin        = (GcbPictographicSequence|GcbZwj),
};

extern const Tsq::Unitable<codepoint_t> g_single_ambig_table;
extern const Tsq::Unitable<codepoint_t> g_double_ambig_table;
extern const Tsq::Unitable<uint16_t, GcbHangulLVT> g_hangul_table;

typedef const Tsq::Unitable<codepoint_t> *MainTablePtr;
