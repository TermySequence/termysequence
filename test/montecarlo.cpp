// Copyright © 2018 TermySequence LLC
//
// SPDX-License-Identifier: GPL-2.0-only

#include "common.h"
#include "cellunit.h"

#include <ctime>

#define ROWLENGTH 100
#define ITERATIONS 1000000

static CellAttributes a, b, c, d;

#define STRSIZE 1024
static char prevstr[STRSIZE];
static char curstr[STRSIZE];
static char iterstr[STRSIZE];

static void printState(char *buf, const TermEventTransfer &t)
{
    const auto &ranges = t.ranges();
    const uint32_t *data = ranges.data();
    const uint32_t *ptr = data;
    char *c = buf;

    for (size_t j = 0; j < ranges.size() / 6; ++j) {
        c += sprintf(c, "\trange %zu: [ %u, %u, %x, %u, %u, %u ]\n",
                     j, ptr[0], ptr[1], ptr[2], ptr[3], ptr[4], ptr[5]);
        ptr += 6;
    }
}

static void getIteration(int &startPos, int &numChars, codepoint_t &codepoint, int &flags)
{
    int rando = random() % 100;

    // 50% of the time: plain update from 1 to rowlength/2
    if (rando < 50) {
        // choose the start position
        startPos = random() % ROWLENGTH;
        // choose the number of chars
        numChars = random() % (ROWLENGTH/2);
        if (numChars + startPos > ROWLENGTH)
            numChars = ROWLENGTH - startPos;

        // choose the character
        codepoint = 65 + random() % 26;
        flags = 0;
    }
    // 25% of the time: flags update from 1 to rowlength/4
    else if (rando < 75) {
        // choose the start position
        startPos = random() % ROWLENGTH;
        // choose the number of chars
        numChars = random() % (ROWLENGTH/4);
        if (numChars + startPos > ROWLENGTH)
            numChars = ROWLENGTH - startPos;

        // choose the character
        codepoint = 65 + random() % 26;
        flags = 1 + random() % 3;
    }
    // 15% of the time: single plain widechar
    else if (rando < 90) {
        // choose the start position
        startPos = random() % ROWLENGTH;
        numChars = 1;

        // choose the character
        codepoint = 0x2573;
        flags = 0;
    }
    // 8% of the time: single flags widechar
    else if (rando < 98) {
        // choose the start position
        startPos = random() % ROWLENGTH;
        numChars = 1;

        // choose the character
        codepoint = 0x254b;
        flags = 1 + random() % 3;
    }
    // 2% of the time: erase a random region from 1 to rowlength/2
    else {
        // choose the start position
        startPos = random() % ROWLENGTH;
        // choose the number of chars
        numChars = random() % (ROWLENGTH/2);
        if (numChars + startPos > ROWLENGTH)
            numChars = ROWLENGTH - startPos;

        codepoint = 0;
        flags = 0;
        snprintf(iterstr, STRSIZE, "erase %d starting at %d", numChars, startPos);
    }
}

static void checkIntegrity(const TermEventTransfer &t)
{
    const auto &ranges = t.ranges();
    unsigned prevend;
    size_t i;

    for (i = 0; i < ranges.size(); i += 6) {
        if (ranges[i] > ranges[i + 1] || (i && ranges[i] <= prevend)) {
            fputs("Integrity failure\n", stderr);
            goto failed;
        }
        if (i && (prevend == ranges[i] - 1) && ranges[i + 2] == ranges[i - 3]) {
            fputs("Flags failure\n", stderr);
            goto failed;
        }

        prevend = ranges[i + 1];
    }

    printState(prevstr, t);
    return;
failed:
    printState(curstr, t);
    fprintf(stderr, "Prev state:\n%s\n", prevstr);
    fprintf(stderr, "Operation: %s\n", iterstr);
    fprintf(stderr, "Cur state:\n%s\n", curstr);
    exit(1);
}

static void test1()
{
    DECLARE_CURSOR(0);

    for (int i = 0; i < ROWLENGTH; ++i)
        row.append(a, ' ', 1);

    checkIntegrity(row);

    for (int i = 0; i < ITERATIONS; ++i) {
        // get start position, number of characters, character, flags
        int startPos, numChars, flags;
        codepoint_t codepoint;

        getIteration(startPos, numChars, codepoint, flags);

        if (codepoint == 0) {
            // perform erase
            row.erase(startPos, startPos + numChars, wl);
            checkIntegrity(row);
            continue;
        }

        // move cursor to start position
        cursor.rx() = startPos;
        row.updateCursor(cursor, wl);

        const CellAttributes &e = flags == 0 ? a :
            (flags == 1 ? b :
             (flags == 2 ? c : d));

        // write the specified number of characters
        for (int j = 0; j < numChars; ++j) {
            row.replace(cursor, e, codepoint, 1, wl);
            ++cursor.rx();
            row.updateCursor(cursor, wl);
        }

        // check integrity
        checkIntegrity(row);
    }
}

int main()
{
    srandom(time(NULL));

    b.flags = 1;
    c.flags = 2;
    d.flags = 3;

    test1();
    return 0;
}
