// Copyright © 2019 TermySequence LLC
//
// SPDX-License-Identifier: GPL-2.0-only

#pragma once

#include "types.h"
#include "flags.h"

//
// This is the plugin interface for implementing a Unicode variant
//
#define UNIPLUGIN_VERSION 1

//
// Encoding boolean parameters (prefixed with + or -)
//
#define TSQ_UNICODE_PARAM_WIDEAMBIG         "wideambig"

//
// Handy constants
//
#define TEXT_SELECTOR   0xFE0E
#define EMOJI_SELECTOR  0xFE0F
#define ZWJ             0x200D

extern "C" {

//
// Plugin description
//
struct UnicodingParams {
    // Name of the variant
    const char *variant;
    // Revision number of the implementation
    uint32_t revision;
    // NULL-terminated array of parameter strings or parameter names
    const char **params;
};

struct UnicodingImpl;

struct UnicodingInfo {
    // Report the plugin interface version supported by the plugin
    int32_t version;
    // Report a NULL-terminated list of supported variants with unique names
    // Report as the params of each entry a list of the parameter names supported
    const UnicodingParams *variants;
    // Report the default (preferred) variant name
    const char *defaultName;

    // Note: all strings reported above must remain fixed for the lifetime of the plugin

    // Operations

    // Create an instance of a variant implementation
    //   Version is the plugin interface version requested by the host
    //   Params is the requested parameters for the instance
    //   Impl is the (pre-zeroed) structure to fill out
    //   Return zero on success, -1 otherwise
    //   Report the actual version and params in the impl structure
    int32_t (*create)(int32_t version, const UnicodingParams *params, UnicodingImpl *impl);
};

//
// State tracker and interface object for an instance of a variant
//
struct UnicodingImpl {
    // Set this to the structure version (=1)
    int32_t version;

    // State fields for widthCategoryOf
    //   These are for implementation private use
    //   These are cleared to zero on the ASCII fastpath
    uint32_t state;
    Tsq::CellFlags flags;
    // Report the sequence of codepoints in the current widthCategoryOf character
    //   These are updated on the ASCII fastpath
    uint32_t len;
    codepoint_t *seq;

    // State fields for widthNext
    // Report the PerCharFlags for the most recent widthNext character
    Tsq::CellFlags nextFlags;
    // Report the sequence of codepoints in the most recent widthNext character
    uint32_t nextLen;
    codepoint_t *nextSeq;

    // Private storage for implementation
    void *privdata;
    // Report the effective parameters for this instance at create time
    UnicodingParams params;

    // Free the structure and all its allocated resources
    void (*teardown)(UnicodingImpl *thiz);

    // Operations

    // Return the width (1 or 2) of the character at pos < end
    int32_t (*widthAt)(const UnicodingImpl *thiz, const char *pos, const char *end);

    // Return the width (1 or 2) of the character at *pos < end
    //   Update *pos to the position past the character
    //   Update nextFlags, nextLen, and nextSeq as appropriate
    int32_t (*widthNext)(UnicodingImpl *thiz, const char **pos, const char *end);

    // Add c to the current widthCategoryOf character or start a new character
    //   Or the PerCharFlags of the most recent character into *flagsor
    //   Update len and seq as appropriate
    //   Return the width category of the code point:
    //     0: code point combines into the current character
    //     -2: code point combines into the current character and increases the width from 1 to 2
    //     1: code point starts a new character with width 1
    //     2: code point starts a new character with width 2
    //     255: ignore this code point entirely
    int32_t (*widthCategoryOf)(UnicodingImpl *thiz, codepoint_t c, Tsq::CellFlags *flagsor);
};

//
// Plugin exports
//

// Export this name with type UnicodingInitFunc
#define UNIPLUGIN_EXPORT_INIT "uniplugin_init"

// Called by the host to initialize the plugin
//   Fill out the info structure with the plugin information
//   Version is the plugin interface version supported by the host
//   Return zero on success, -1 otherwise
extern int32_t
uniplugin_init(int32_t version, UnicodingInfo *info);

typedef typeof(uniplugin_init) *UnicodingInitFunc;
typedef typeof(UnicodingInfo::create) UnicodingCreateFunc;

}
