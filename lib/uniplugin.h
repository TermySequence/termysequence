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
#define TSQ_UNICODE_PARAM_EMOJI             "emoji"
#define TSQ_UNICODE_PARAM_WIDEAMBIG         "wideambig"

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
// State tracker and interface object for an instance of a variant
//
struct UnicodingImpl {
    int32_t version;

    codepoint_t *seq;
    uint32_t len;
    Tsq::CellFlags flags;

    codepoint_t *nextSeq;
    uint32_t nextLen;
    Tsq::CellFlags nextFlags;

    // Private storage for implementation
    void *privdata;
    // Effective parameters for this instance
    UnicodingParams params;

    // Free the structure and all its allocated resources
    void (*teardown)(UnicodingImpl *thiz);

    // Operations
    int32_t (*widthAt)(const UnicodingImpl *thiz, const char *pos, const char *end);
    int32_t (*widthNext)(UnicodingImpl *thiz, const char **pos, const char *end);
    int32_t (*widthCategoryOf)(UnicodingImpl *thiz, codepoint_t c, Tsq::CellFlags *flagsor);
};

//
// Plugin description
//
struct UnicodingVariant {
    const char *variant;
    int32_t revision;
};

struct UnicodingInfo {
    int32_t version;
    const UnicodingVariant *variants;
    const char **params;
    const char *defaultName;
    int32_t (*create)(int32_t version, const UnicodingParams *params, UnicodingImpl *impl);
};

//
// Plugin exports
//
#define UNIPLUGIN_EXPORT_INIT "uniplugin_init"

extern int32_t
uniplugin_init(int32_t version, UnicodingInfo *info);

typedef typeof(uniplugin_init) *UnicodingInitFunc;
typedef typeof(UnicodingInfo::create) UnicodingCreateFunc;

}
