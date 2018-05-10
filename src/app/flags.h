// Copyright © 2018 TermySequence LLC
//
// SPDX-License-Identifier: GPL-2.0-only

#pragma once

#include "lib/flags.h"

namespace Tsqt {

    enum AdditionalTermFlag : uint64_t {
        AltScroll                     = (Tsq::AltScrollMouseMode|Tsq::AppScreen),
        AnyMod                        = (uint64_t)1 << 24,
        Shift                         = (uint64_t)1 << 25, // Qt::ShiftModifier
        Control                       = (uint64_t)1 << 26, // Qt::ControlModifier
        Alt                           = (uint64_t)1 << 27, // Qt::AltModifier
        Meta                          = (uint64_t)1 << 28, // Qt::MetaModifier
        ModMask                       = (Shift|Control|Alt|Meta),
        KeyPad                        = (uint64_t)1 << 29, // Qt::KeypadModifier
        CommandMode                   = (uint64_t)1 << 31,
        SelectMode                    = (uint64_t)1 << 63,
        DefaultKeymapFeatures         = Alt,
    };

    enum AdditionalCellFlag : uint64_t {
        Annotation                    = (uint64_t)1 << 32,
        SearchLine                    = (uint64_t)1 << 33,
        SearchText                    = (uint64_t)1 << 34,
        ActivePrompt                  = (uint64_t)1 << 35,
        Selected                      = (uint64_t)1 << 36,
        SolidCursor                   = (uint64_t)1 << 37,
        BoxedCursor                   = (uint64_t)1 << 38,
        PaintU2500                    = (uint64_t)1 << 39,
        UnderlineDummy                = (uint64_t)1 << 40,
        // Groupings
        PaintOverride = (Tsq::EmojiChar|PaintU2500),
        Special       = (Annotation|SearchLine|SearchText|ActivePrompt|Tsq::Command|Tsq::Prompt),
        VisibleBg     = (Special|SolidCursor|Selected|Tsq::Inverse|Tsq::Bg),
        Visible       = (VisibleBg|Tsq::Underline),
        TermFill      = (VisibleBg|Tsq::Fg),
        ThumbFill     = (SolidCursor|Tsq::Command|Tsq::Prompt|Tsq::Inverse|Tsq::Bg),
    };

    enum AdditionalLineFlag : uint32_t {
        Downloaded                    = 1u << 31,
        NoSelect                      = 1u << 30,
        SearchHit                     = 1u << 29,
    };

    enum AdditionalRegionFlag : uint32_t {
        Updating                      = 1u << 31,
        Inline                        = 1u << 30,
        Semantic                      = 1u << 29,
        HaveJobIcon                   = 1u << 28,
        HaveJobExitCode               = 1u << 27,
        IgnoredCommand                = 1u << 26,
    };

    enum AdditionalGitStatusFlag : uint32_t {
        GitStatusAnyIndex             = 0x1f,
        GitStatusAnyWorking           = 0xf80,
    };

    typedef uint64_t CellFlags;
}
