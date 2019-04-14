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
// Encoding parameter names (parameter strings take the form name[=value])
//
#define TSQ_UNICODE_PARAM_REVISION          "v"
#define TSQ_UNICODE_PARAM_LOCALE            "locale"
#define TSQ_UNICODE_PARAM_STDLIB            "stdlib"
#define TSQ_UNICODE_PARAM_WIDEAMBIG         "+wideambig"

// Maximum codepoints in a grapheme cluster (128 or less)
#define MAX_CLUSTER_SIZE 16

//
// Handy constants
//
#define TEXT_SELECTOR   0xFE0E
#define EMOJI_SELECTOR  0xFE0F
#define ZWJ             0x200D
#define REPLACEMENT_CH  0xFFFD

extern "C" {

//
// Plugin description
//
struct UnicodingParams;
struct UnicodingImpl;

enum UnicodingVariantFlags {
    VFNoFlags = 0u,
    VFHidden = 1u,
    VFPlatformDependent = 2u,
};

struct UnicodingVariant {
    // String prefix to match against incoming encoding names
    const char *prefix;
    // Flags that configure variant behavior
    uint64_t flags;
    // NULL-terminated array of supported parameter names
    const char **params;

    // Operations

    // Create an instance of a variant implementation
    //   Version is the plugin interface version requested by the host
    //   Params is the requested variant name and parameters
    //   Impl is the (pre-zeroed) structure to fill out
    //   Return zero on success, -1 otherwise
    //   Report the actual variant name and parameters in the impl structure
    int32_t (*create)(int32_t version, const UnicodingParams *params, int64_t privarg,
                      UnicodingImpl *impl);

    // Private data given to create method
    int64_t privarg;
};

struct UnicodingInfo {
    // Report the plugin interface version supported by the plugin
    int32_t version;
    // Report a list of supported variants terminated by a NULL prefix name
    // Note: variant information must remain in memory for the plugin lifetime
    const UnicodingVariant *variants;
};

//
// State tracker and interface object for an instance of a variant
//
struct UnicodingParams {
    // Name of the variant
    const char *variant;
    // NULL-terminated array of parameter strings
    const char **params;
};

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
    int64_t privdata;
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
    //     0: codepoint combines into the current character
    //     1: codepoint starts a new character of width 1
    //     2: codepoint starts a new character of width 2
    //     -2: substitute the codepoint sequence starting at nextLen, of width 1
    //     -3: substitute the codepoint sequence starting at nextLen, of width 2
    //     -258: back up 1 and replay the codepoint sequence of width 2
    //     -99999: ignore the codepoint
    //
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
//   Return the version value of the filled-out structure or -1 on failure
typedef int32_t (*UnicodingInitFunc)(int32_t version, UnicodingInfo *info);

}
