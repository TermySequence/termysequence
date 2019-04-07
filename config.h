// Copyright © 2018 TermySequence LLC
//
// SPDX-License-Identifier: GPL-2.0-only

#pragma once

/* Default terminal rows */
#define TERM_ROWS 24
/* Default terminal columns */
#define TERM_COLS 80
/* Minimum terminal rows */
#define TERM_MIN_ROWS 8
/* Maximum terminal rows */
#define TERM_MAX_ROWS 1024
/* Minimum terminal columns */
#define TERM_MIN_COLS 16
/* Maximum terminal columns */
#define TERM_MAX_COLS 1024
/* Default buffer capacity order */
#define TERM_DEF_CAPORDER 12
/* Maximum buffer capacity order */
#define TERM_MAX_CAPORDER 30
/* Initial caporder reported by proxy while waiting for real one */
#define TERM_INIT_CAPORDER 10

/* Default terminal command */
#define TERM_COMMAND "bash\x1f""bash\x1f-l"
/* Default terminal environment */
#define TERM_ENVIRON "+TERM=xterm-256color"

/* Maximum number of server connections */
#define MAX_CONNECTIONS 512
/* Number of file descriptors to try closing on startup */
#define OS_DESCRIPTOR_COUNT 128
/* Maximum length of socket and pid file paths */
#define SOCKET_PATHLEN 104

/* Server version used in protocol handshake */
#define SERVER_VERSION 1
/* Client version used in protocol handshake */
#define CLIENT_VERSION 1

/* Maximum size of control sequence fields (including osc fields) */
#define SEQUENCE_FIELD_MAX 8388608
/* Maximum number of non-handshake bytes before connect will fail */
#define CONNECT_HANDSHAKE_MAX 8192

/* Size of buffer for reading from files */
#define FILE_BUFSIZE 524288
/* Size of buffer for reading from terminals */
#define TERM_BUFSIZE 65536
/* Size of buffer for reading from connections */
#define READER_BUFSIZE 212992
/* Size of buffer for writing to connections */
#define WRITER_BUFSIZE 65536
/* Default starting length for body buffer */
#define BODY_DEF_LENGTH 65536
/* Maximum length of bodies read from connections */
#define BODY_MAX_LENGTH 16777216
/* Buffered amount that causes warning */
#define BUFFER_WARN_THRESHOLD 1048576
/* Maximum size of content that can be fetched without a task */
#define IMAGE_SIZE_THRESHOLD 524288
/* Maximum size of key-value attribute lines */
#define ATTRIBUTE_MAX_LENGTH 4096
/* Maximum size of uncompressed avatar images (plus 1) */
#define AVATAR_MAX_SIZE 262144

/* Terminal process monitor initial wait in milliseconds (power of 2) */
#define IDLE_INITIAL_TIMEOUT 250
/* Terminal process monitor final wait in milliseconds (power of 2) */
#define IDLE_LAST_TIMEOUT 8000
/* Time (non-idle) before ratelimiting starts in deciseconds */
#define RATELIMIT_THRESHOLD 20
/* Time between ratelimited updates in deciseconds */
#define RATELIMIT_INTERVAL 5
/* Default minimum autoclose run time in milliseconds */
#define DEFAULT_AUTOCLOSE_TIME 1000
/* Timeout for file tasks */
#define FILETASK_IDLE_TIME 60000
/* Timeout for process task (after sending signal) */
#define RUNTASK_IDLE_TIME 5000
/* Timeout for attribute scripts and monitor --initial */
#define ATTRIBUTE_SCRIPT_TIMEOUT 4000
/* Warning threshold for scripts */
#define ATTRIBUTE_SCRIPT_WARN    2000
/* Timeout for querying emulator attributes using osc sequence */
#define OSC_QUERY_TIMEOUT 1000
/* Default keepalive time (update: connect.1) */
#define KEEPALIVE_DEFAULT 25000
/* Minimum keepalive time (update: connect.1) */
#define KEEPALIVE_MIN 5000

/* Maximum codepoints in a grapheme cluster (128 or less) */
#define MAX_CLUSTER_SIZE 16
/* Maximum lines in reported commands */
#define MAX_COMMAND_LINES 5

/* Terminal file monitor readdir batch size */
#define FILEMON_BATCH_SIZE 25
/* Terminal file monitor default file limit */
#define FILEMON_DEFAULT_LIMIT 250
/* Mount maximum number of cached filehandles */
#define MOUNT_MAX_HANDLES 32
/* Maximum number of cached git directories */
#define GIT_CACHE_MAX 1024
/* Time before expiring a cached git directory */
#define GIT_CACHE_TIMEOUT 300000
