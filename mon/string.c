// Copyright © 2018 TermySequence LLC
//
// SPDX-License-Identifier: GPL-2.0-only

#include "common.h"
#include "monitor.h"

#include <stdio.h>
#include <unistd.h>

#define STARTING_BUFCAP 1024
#define STARTING_MAPCAP 32

void
stringInit(struct mon_string *str)
{
    str->map = malloc(STARTING_MAPCAP * sizeof(char*));
    str->mapsize = 0;
    str->mapcap = STARTING_MAPCAP;
}

void
stringReset(struct mon_string *str)
{
    str->buf = malloc(STARTING_BUFCAP);
    str->buf[0] = '\0';
    str->bufsize = 1;
    str->bufcap = STARTING_BUFCAP;
}

static void
stringExpand(struct mon_string *str)
{
    while (str->bufcap < str->bufsize) {
        str->bufcap *= 2;
        str->buf = realloc(str->buf, str->bufcap);
    }
    while (str->mapcap < str->mapsize) {
        str->mapcap *= 2;
        str->map = realloc(str->map, str->mapcap * sizeof(char*));
    }
}

void
stringInsert(struct mon_string *str, const char *key, const char *val)
{
    unsigned i, pos, addKey = 1;
    char **ptr;

    for (i = 0; i < str->mapsize; i += 2)
        if (!strcmp(str->map[i], key))
        {
            ptr = str->map + i + 1;
            if (*ptr && !strcmp(*ptr, val))
                return;

            free(*ptr);
            *ptr = strdup(val);
            addKey = 0;
            break;
        }

    pos = str->bufsize - 1;
    str->bufsize += strlen(key) + strlen(val) + 2;
    if (addKey)
        str->mapsize += 2;

    stringExpand(str);

    if (addKey) {
        ptr = str->map + str->mapsize - 2;
        *ptr = strdup(key);
        ++ptr;
        *ptr = strdup(val);
    }

    sprintf(str->buf + pos, "%s=%s\n", key, val);
}

void
stringRemove(struct mon_string *str, const char *key)
{
    unsigned i, pos;

    for (i = 0; i < str->mapsize; i += 2)
        if (!strcmp(str->map[i], key))
        {
            char **ptr = str->map + i + 1;
            if (!*ptr)
                return;

            free(*ptr);
            *ptr = NULL;
            goto next;
        }

    return;
next:
    pos = str->bufsize - 1;
    str->bufsize += strlen(key) + 1;
    stringExpand(str);

    sprintf(str->buf + pos, "%s\n", key);
}

void
stringPrint(struct mon_string *str)
{
    unsigned total = str->bufsize - 1;
    unsigned sent = 0;

    while (sent < total) {
        ssize_t rc = write(STDOUT_FILENO, str->buf + sent, total - sent);
        if (rc <= 0)
            break;
        sent += rc;
    }

    free(str->buf);
}

void
stringFree(struct mon_string *str)
{
    unsigned i;

    for (i = 0; i < str->mapsize; ++i)
        free(str->map[i]);

    free(str->map);
}
