// Copyright © 2018 TermySequence LLC
//
// SPDX-License-Identifier: GPL-2.0-only

#pragma once

typedef uint32_t Color;

#define MAKE_COLOR(r, g, b) (0xff000000 | ((r & 0xff) << 16) | ((g & 0xff) << 8) | (b & 0xff))
#define COLOR_RED(rgb) (rgb >> 16 & 0xff)
#define COLOR_GREEN(rgb) (rgb >> 8 & 0xff)
#define COLOR_BLUE(rgb) (rgb & 0xff)
