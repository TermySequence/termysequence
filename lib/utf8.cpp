// Copyright © 2018 TermySequence LLC
//
// SPDX-License-Identifier: GPL-2.0-only

#include "common.h"
#include "utf8.h"

namespace utf8 {
    template bool is_valid(const char *begin, const char *end);
    template bool is_valid(std::string::const_iterator begin,
                           std::string::const_iterator end);

    template uint32_t peek_next(const char *ptr, const char *end);

    namespace unchecked {
        template uint32_t peek_next(const char *ptr);
        template uint32_t next(const char *&ptr);
    }
}
