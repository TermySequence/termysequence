// Copyright © 2018 TermySequence LLC
//
// SPDX-License-Identifier: GPL-2.0-only

#pragma once

// UTF8-CPP library with explicit instantiations

#include <utf8.h>

namespace utf8 {
    extern template bool is_valid(const char *begin, const char *end);
    extern template bool is_valid(std::string::const_iterator begin,
                                  std::string::const_iterator end);

    extern template uint32_t peek_next(const char *ptr, const char *end);

    namespace unchecked {
        extern template uint32_t peek_next(const char *ptr);
        extern template uint32_t next(const char *&ptr);
    }
}
