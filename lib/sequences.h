// Copyright © 2018 TermySequence LLC
//
// SPDX-License-Identifier: GPL-2.0-only

#pragma once

#define TSQ_ESC                 '\x1b'
#define TSQ_CSI                 '\x9b'
#define TSQ_ST                  '\x9c'
#define TSQ_ST7                 "\x1b\\"
#define TSQ_OSC                 '\x9d'

#define TSQ_BPM_START           "\x1b[200~"
#define TSQ_BPM_START_LEN       6
#define TSQ_BPM_END             "\x1b[201~"
#define TSQ_BPM_END_LEN         6

#define TSQ_HANDSHAKE           "\x9d""511;"
#define TSQ_HANDSHAKE_LEN       5
#define TSQ_HANDSHAKE7          "\x1b]511;"
#define TSQ_HANDSHAKE7_LEN      6
#define TSQ_DATAPREFIX          "\x9d""512;"
#define TSQ_DATAPREFIX_LEN      5
#define TSQ_DATAPREFIX7         "\x1b]512;"
#define TSQ_DATAPREFIX7_LEN     6
#define TSQ_GETVARPREFIX        "\x1b]513;"
#define TSQ_GETVARPREFIX_LEN    6
#define TSQ_SETVARPREFIX        "\x1b]514;"
#define TSQ_SETVARPREFIX_LEN    6

/* Chunk size to use in the terminal encoding */
#define TERM_CHUNKSIZE 1024
#define TERM_PAYLOADSIZE ((TERM_CHUNKSIZE - 8) * 3 / 4)

#define TSQ_CURSOR_UP           "\x1b[A"
#define TSQ_CURSOR_DOWN         "\x1b[B"
#define TSQ_APPCURSOR_UP        "\x1bOA"
#define TSQ_APPCURSOR_DOWN      "\x1bOB"
#define TSQ_FOCUS_IN            "\x1b[I"
#define TSQ_FOCUS_OUT           "\x1b[O"

#define TSQ_RAW_DISCONNECT      "\x05\x00\x00\x00\x00\x00\x00"
#define TSQ_RAW_DISCONNECT_LEN  8
#define TSQ_TERM_DISCONNECT     "\x1b]512;BQAAAAAAAAA\x07"
#define TSQ_TERM_DISCONNECT_LEN 18

#define TSQ_RAW_KEEPALIVE       "\x06\x00\x00\x00\x00\x00\x00"
#define TSQ_RAW_KEEPALIVE_LEN   8

/* Channel test, containing escape sequences likely to be a problem */
#define TSQ_CHANNEL_TEST "\r~.\r\x1e.\x1d\x1d\x1d"
