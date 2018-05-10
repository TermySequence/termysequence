// Copyright © 2018 TermySequence LLC
//
// SPDX-License-Identifier: GPL-2.0-only

#include "common.h"
#include "os/attr.h"
#include <cstdio>

static void
identityTest()
{
    Tsq::Uuid result;
    std::vector<int> pids;

    osIdentity(result, pids);

    printf("Identity: %s\n", result.str().c_str());

    for (int pid: pids)
        printf("Pid: %d\n", pid);
}

static void
attributeTest()
{
    std::unordered_map<std::string,std::string> map;
    std::vector<int> pids;

    osAttributes(map, pids, false);

    for (auto &i: map)
        printf("Key-Value: %s -> %s\n", i.first.c_str(), i.second.c_str());

    for (int pid: pids)
        printf("Pid: %d\n", pid);
}

int main()
{
    identityTest();
    attributeTest();
    return 0;
}
