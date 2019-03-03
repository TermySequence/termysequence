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
// Handy constants
//
#define TEXT_SELECTOR   0xFE0E
#define EMOJI_SELECTOR  0xFE0F
#define ZWJ             0x200D

extern "C" {

//
// Parameters for a variant
//
struct UnicodingParams {
    const char *variant;
    const char **params;
    int32_t revision;
};

//
// Allocate and return this from the create function
//
struct UnicodingImpl {
    int32_t version;

    codepoint_t *seq;
    uint32_t len;
    Tsq::CellFlags flags;

    codepoint_t *nextSeq;
    uint32_t nextLen;
    Tsq::CellFlags nextFlags;

    // Effective parameters for this instance
    UnicodingParams params;
    // Private storage for implementation
    void *privdata;

    // Free the structure and all its allocated resources
    void (*teardown)(UnicodingImpl *thiz);

    // Operations
    int32_t (*widthAt)(const UnicodingImpl *thiz, const char *pos, const char *end);
    int32_t (*widthNext)(UnicodingImpl *thiz, const char **pos, const char *end);
    void (*next)(UnicodingImpl *thiz, const char **pos, const char *end);
    int32_t (*widthCategoryOf)(UnicodingImpl *thiz, codepoint_t c, Tsq::CellFlags *flagsor);
};

//
// Plugin exports:
// Export as "variants" a NULL-terminated array of supported variant names
// Export as "create" a function of this type
//
typedef int32_t (*CreateFunc)(int32_t version, const UnicodingParams *params,
                              UnicodingImpl *impl);
}
