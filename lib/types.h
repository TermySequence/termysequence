// Copyright © 2018 TermySequence LLC
//
// SPDX-License-Identifier: GPL-2.0-only

#pragma once

// Unicode codepoint
typedef uint32_t codepoint_t;
// Logical row index
typedef uint64_t index_t;
// Logical column index
typedef uint32_t column_t;
// Region ID
typedef uint32_t regionid_t;
// Buffer and region ID combined
typedef uint64_t bufreg_t;
// Portfwd channel ID
typedef uint32_t portfwd_t;
// Content ID
typedef uint64_t contentid_t;

#define INVALID_INDEX           UINT64_MAX
#define INVALID_COLUMN          UINT32_MAX
#define INVALID_REGION_ID       0
#define INVALID_MODTIME         INT32_MIN
#define INVALID_WALLTIME        INT64_MIN
#define INVALID_PORTFWD         0
#define INVALID_CONTENT_ID      0

#define MAKE_BUFREG(bufid, region) ((bufreg_t)bufid << 32 | region)
#define BUFREG_BUF(bufreg) (bufreg >> 32)
#define BUFREG_REG(bufreg) (regionid_t)(bufreg & 0xffffffff)
