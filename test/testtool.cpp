// Copyright © 2018 TermySequence LLC
//
// SPDX-License-Identifier: GPL-2.0-only

#include "common.h"
#include <vector>

extern "C" {
#include "util.h"
}

static void
usage()
{
    fputs("Usage:\n"
          "\ttesttool -- <cmd>...\n"
          "\ttesttool -\n"
          "\ttesttool -f file\n", stderr);
    exit(1);
}

static void
die(const char *msg)
{
    tty_unraw(STDIN_FILENO);
    fprintf(stderr, "%s: %m\n", msg);
    exit(2);
}

static void
command(const char *cmd)
{
    std::string output(cmd);

    switch (*cmd) {
    case '\0':
        output.push_back('\n');
        break;
    case '-':
        if (output.size() == 1) {
            // Go raw
            tty_raw(STDIN_FILENO);
            return;
        }
        break;
    case '+':
        if (output.size() == 1) {
            // Return from raw
            tty_unraw(STDIN_FILENO);
            return;
        }
        break;
    case '<':
        if (output.size() == 1) {
            // Read responses
            read_replies(STDIN_FILENO, 1);
            return;
        }
        break;
    case '[':
    case ']':
        output.insert(0, 1, '\x1b');
        break;
    case '*':
        output[0] = '\x1b';
        break;
    case '(':
        if (output.back() == ')') {
            char *end;
            long code = strtol(output.c_str() + 1, &end, 10);
            if (end[1] == '\0') {
                output.clear();
                output.push_back((char)code);
            }
        }
        break;
    case '@':
        sleep(atoi(cmd + 1));
        return;
    case ',':
        output.erase(0, 1);
        output.append("\r\n");
        break;
    default:
        break;
    }

    size_t left = output.size();
    const char *ptr = output.data();

    while (left) {
        ssize_t rc = write(STDOUT_FILENO, ptr, left);
        if (rc < 0)
            die("write");
        ptr += rc;
        left -= rc;
    }
}

static void
file(FILE *f)
{
    char buf[1024];
    std::vector<std::string> lines;

    while (fgets(buf, sizeof(buf), f)) {
        std::string line(buf);
        if (!line.empty()) {
            if (line.front() == '#')
                continue;
            if (line.back() == '\n')
                line.pop_back();
        }

        if (!line.empty())
            lines.emplace_back(std::move(line));
    }

    for (const auto &cmd: lines)
        command(cmd.data());
}

int
main(int argc, char **argv)
{
    if (argc == 1)
        usage();

    setSignalHandler();

    if (!strcmp(argv[1], "-f")) {
        if (argc != 3)
            usage();

        FILE *f = fopen(argv[2], "r");
        if (!f)
            die(argv[2]);
        file(f);
        fclose(f);
    }
    else if (!strcmp(argv[1], "--")) {
        for (int i = 2; i < argc; ++i)
            command(argv[i]);
    }
    else if (!strcmp(argv[1], "-")) {
        if (argc != 2)
            usage();

        file(stdin);
    }
    else {
        usage();
    }

    tty_unraw(STDIN_FILENO);
    return 0;
}
