// Copyright © 2018 TermySequence LLC
//
// SPDX-License-Identifier: GPL-2.0-only

#pragma once

#ifdef PLUGIN
#include "../vendor/utf8cpp/utf8.h"
#else

// UTF8-CPP library
#include <utf8.h>

namespace utf8 {
    // Explicit instantiations
    extern template bool is_valid(const char *begin, const char *end);
    extern template bool is_valid(std::string::const_iterator begin,
                                  std::string::const_iterator end);

    extern template uint32_t peek_next(const char *ptr, const char *end);

    namespace unchecked {
        extern template uint32_t peek_next(const char *ptr);
        extern template uint32_t next(const char *&ptr);
    }

    // Custom functions
    template <typename octet_iterator>
    uint32_t validating_next(octet_iterator& it, octet_iterator end)
    {
        uint32_t cp = 0;
        internal::utf_error err_code = utf8::internal::validate_next(it, end, cp);
        switch (err_code) {
            case internal::UTF8_OK :
                break;
            case internal::NOT_ENOUGH_ROOM :
                throw not_enough_room();
            case internal::INVALID_LEAD :
                ++it;
                cp = (uint32_t)-1;
                break;
            case internal::INCOMPLETE_SEQUENCE :
            case internal::OVERLONG_SEQUENCE :
            case internal::INVALID_CODE_POINT :
                ++it;
                // just one replacement mark for the sequence
                while (it != end && utf8::internal::is_trail(*it))
                    ++it;
                cp = (uint32_t)-1;
                break;
        }
        return cp;
    }
}

#endif
