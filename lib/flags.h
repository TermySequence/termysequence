// Copyright © 2018 TermySequence LLC
//
// SPDX-License-Identifier: GPL-2.0-only

#pragma once

namespace Tsq {
    enum TermFlag : uint64_t {
        // Base flags
        None                          = (uint64_t)0,
        Ansi                          = (uint64_t)1,
        NewLine                       = (uint64_t)1 << 1,
        AppCuKeys                     = (uint64_t)1 << 2,
        AppScreen                     = (uint64_t)1 << 3,
        AppKeyPad                     = (uint64_t)1 << 4,
        ReverseVideo                  = (uint64_t)1 << 5,
        BlinkSeen                     = (uint64_t)1 << 6,
        HardScrollLock                = (uint64_t)1 << 7,
        SoftScrollLock                = (uint64_t)1 << 8,
        KeyboardLock                  = (uint64_t)1 << 9,
        SendReceive                   = (uint64_t)1 << 10,
        InsertMode                    = (uint64_t)1 << 11,
        LeftRightMarginMode           = (uint64_t)1 << 12,
        OriginMode                    = (uint64_t)1 << 13,
        SmoothScrolling               = (uint64_t)1 << 14,
        Autowrap                      = (uint64_t)1 << 15,
        ReverseAutowrap               = (uint64_t)1 << 16,
        Autorepeat                    = (uint64_t)1 << 17,
        AllowColumnChange             = (uint64_t)1 << 18,
        Controls8Bit                  = (uint64_t)1 << 19,
        BracketedPasteMode            = (uint64_t)1 << 20,
        CursorVisible                 = (uint64_t)1 << 21,

        // Mouse flags
        X10MouseMode                  = (uint64_t)1 << 32,
        NormalMouseMode               = (uint64_t)1 << 33,
        HighlightMouseMode            = (uint64_t)1 << 34,
        ButtonEventMouseMode          = (uint64_t)1 << 35,
        AnyEventMouseMode             = (uint64_t)1 << 36,
        MouseModeMask                 = (uint64_t)31 << 32,
        FocusEventMode                = (uint64_t)1 << 37,
        Utf8ExtMouseMode              = (uint64_t)1 << 38,
        SgrExtMouseMode               = (uint64_t)1 << 39,
        UrxvtExtMouseMode             = (uint64_t)1 << 40,
        ExtMouseModeMask              = (uint64_t)7 << 38,
        AltScrollMouseMode            = (uint64_t)1 << 41,

        // Misc flags
        TitleModeSetHex               = (uint64_t)1 << 45,
        TitleModeQueryHex             = (uint64_t)1 << 46,
        RateLimited                   = (uint64_t)1 << 47,

        DefaultTermFlags = (Ansi|SendReceive|Autowrap|Autorepeat|CursorVisible),
        // Bits reserved for client use
        LocalTermMask                 = (uint64_t)0xff000000ff000000,
    };

    enum MouseEventFlag : uint32_t {
        MouseRelease                  = 1u << 8,
        MouseMotion                   = 1u << 9,
        MouseShift                    = 1u << 12,
        MouseMeta                     = 1u << 13,
        MouseControl                  = 1u << 14,
        MouseButtonMask               = 0xff,
    };

    enum CellFlag : uint32_t {
        Fg                            = 1u,
        FgIndex                       = 1u << 1,
        Bg                            = 1u << 2,
        BgIndex                       = 1u << 3,
        Bold                          = 1u << 4,
        Faint                         = 1u << 5,
        Italics                       = 1u << 6,
        Underline                     = 1u << 7,
        DblUnderline                  = 1u << 8,
        Blink                         = 1u << 9,
        FastBlink                     = 1u << 10,
        Inverse                       = 1u << 11,
        Invisible                     = 1u << 12,
        Strikethrough                 = 1u << 13,
        Framed                        = 1u << 14,
        Encircled                     = 1u << 15,
        Overline                      = 1u << 16,
        AltFont0                      = 1u << 17,
        AltFont1                      = 1u << 18,
        AltFont2                      = 1u << 19,
        FontMask                      = 7u << 17,
        All                           = 0xfffff,
        Protected                     = 1u << 24,
        Highlighted                   = 1u << 25,
        Prompt                        = 1u << 26,
        Command                       = 1u << 27,
        Hyperlink                     = 1u << 28,
        EmojiChar                     = 1u << 30,
        DblWidthChar                  = 1u << 31,
        PerCharFlags                  = 3u << 30,
    };

    enum LineFlag : uint32_t {
        // Leave room for 1-byte buffer id
        NoLineFlags                   = 0,
        Continuation                  = 1u << 8,
        DblWidthLine                  = 1u << 9,
        DblTopLine                    = 1u << 10,
        DblBottomLine                 = 1u << 11,
        DblLineMask                   = 7u << 9,

        ServerLineMask                = 0xff00,
        // Bits reserved for client use
        LocalLineMask                 = 0xffff0000,
    };

    enum CursorFlag : uint32_t {
        // Leave room for 1-byte sub-position
        CursorPastEnd                 = 1u << 8,
        OnDoubleLeft                  = 1u << 9,
        OnDoubleRight                 = 1u << 10,
    };

    enum RegionFlag : uint32_t {
        HasStart                      = 1u,
        HasEnd                        = 1u << 1,
        Deleted                       = 1u << 2,
        Overwritten                   = 1u << 3,
        HasPrompt                     = 1u << 4,
        HasCommand                    = 1u << 5,
        EmptyCommand                  = 1u << 6,
        HasOutput                     = 1u << 7,

        // Bits reserved for client use
        LocalRegionMask     = 0xffff0000,
    };

    enum ResetFlag : uint32_t {
        ResetEmulator                 = 1u,
        ClearScrollback               = 1u << 1,
        ClearScreen                   = 1u << 2,
        FormFeed                      = 1u << 3,
    };

    enum ClientFlag : uint32_t {
        TakeOwnership                 = 1u,
    };

    enum TermiosInputFlag : uint32_t {
        TermiosIGNBRK                 = 1u,
        TermiosBRKINT                 = 1u << 1,
        TermiosIGNPAR                 = 1u << 2,
        TermiosPARMRK                 = 1u << 3,
        TermiosINPCK                  = 1u << 4,
        TermiosISTRIP                 = 1u << 5,
        TermiosINLCR                  = 1u << 6,
        TermiosIGNCR                  = 1u << 7,
        TermiosICRNL                  = 1u << 8,
        TermiosIUCLC                  = 1u << 9,
        TermiosIXON                   = 1u << 10,
        TermiosIXANY                  = 1u << 11,
        TermiosIXOFF                  = 1u << 12,
    };

    enum TermiosOutputFlag : uint32_t {
        TermiosOPOST                  = 1u,
        TermiosOLCUC                  = 1u << 1,
        TermiosONLCR                  = 1u << 2,
        TermiosOCRNL                  = 1u << 3,
        TermiosONOCR                  = 1u << 4,
        TermiosONLRET                 = 1u << 5,
        TermiosOXTABS                 = 3u << 11,
    };

    enum TermiosLocalFlag : uint32_t {
        TermiosISIG                   = 1u,
        TermiosICANON                 = 1u << 1,
        TermiosECHO                   = 1u << 3,
        TermiosECHOE                  = 1u << 4,
        TermiosECHOK                  = 1u << 5,
        TermiosECHONL                 = 1u << 6,
        TermiosNOFLSH                 = 1u << 7,
        TermiosTOSTOP                 = 1u << 8,
        TermiosECHOCTL                = 1u << 9,
        TermiosECHOPRT                = 1u << 10,
        TermiosECHOKE                 = 1u << 11,
        TermiosFLUSHO                 = 1u << 12,
        TermiosPENDIN                 = 1u << 14,
        TermiosIEXTEN                 = 1u << 15,
    };

    enum TermiosChars {
        TermiosVEOF                   = 0,
        TermiosVEOL                   = 1,
        TermiosVEOL2                  = 2,
        TermiosVERASE                 = 3,
        TermiosVWERASE                = 4,
        TermiosVKILL                  = 5,
        TermiosVREPRINT               = 6,
        // 7 is unused
        TermiosVINTR                  = 8,
        TermiosVQUIT                  = 9,
        TermiosVSUSP                  = 10,
        TermiosVDSUSP                 = 11,
        TermiosVSTART                 = 12,
        TermiosVSTOP                  = 13,
        TermiosVLNEXT                 = 14,
        TermiosVDISCARD               = 15,
        TermiosVMIN                   = 16,
        TermiosVTIME                  = 17,
        TermiosVSTATUS                = 18,
        // 19 is unused
        TermiosNChars                 = 20,
    };

    // libgit2 status flags
    enum GitStatusFlag : uint32_t {
        GitStatusINew                 = 1u,
        GitStatusIModified            = 1u << 1,
        GitStatusIDeleted             = 1u << 2,
        GitStatusIRenamed             = 1u << 3,
        GitStatusIRetyped             = 1u << 4,
        GitStatusWNew                 = 1u << 7,
        GitStatusWModified            = 1u << 8,
        GitStatusWDeleted             = 1u << 9,
        GitStatusWRetyped             = 1u << 10,
        GitStatusWRenamed             = 1u << 11,
        GitStatusIgnored              = 1u << 14,
        GitStatusUnmerged             = 1u << 15,
    };

    typedef uint64_t TermFlags;
    typedef uint32_t MouseEventFlags;
    typedef uint32_t CellFlags;
    typedef uint32_t LineFlags;
    typedef uint32_t CursorFlags;
    typedef uint32_t RegionFlags;
    typedef uint32_t ResetFlags;
    typedef uint32_t ClientFlags;
    typedef uint32_t TermiosFlags;
    typedef uint32_t GitStatusFlags;
}
