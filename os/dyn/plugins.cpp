// Copyright © 2019 TermySequence LLC
//
// SPDX-License-Identifier: GPL-2.0-only

#include "common.h"
#include "os/plugins.h"
#include "os/encoding.h"
#include "os/fd.h"
#include "os/logging.h"

#include <sys/types.h>
#include <dirent.h>
#include <dlfcn.h>

#ifdef __linux__
#define SO_SUFFIX ".so"
#else /* Mac OS X */
#define SO_SUFFIX ".dylib"
#endif

void
osLoadPlugins()
{
    // Register the compiled-in variants first
    TermUnicoding::registerPlugin(uniplugin_termy_init);
    TermUnicoding::registerPlugin(uniplugin_syswc_init);

    DIR *dir;
    int rc = osOpenDir(DATADIR "/" SERVER_NAME "/plugins", &dir, true);
    FileInfo info(rc);
    while (rc != -1) {
        switch (rc = osReadDir(dir, &info) != -1) {
        case -1:
            closedir(dir);
            break;
        case 1:
            const size_t n = sizeof(SO_SUFFIX) - 1;
            if (info.name.size() <= n)
                break;
            if (info.name.compare(info.name.size() - n, n, SO_SUFFIX, n))
                break;

            // Try to load this plugin
            void *h = dlopen(info.name.c_str(), RTLD_LAZY);
            if (h && (h = dlsym(h, UNIPLUGIN_EXPORT_INIT))) {
                TermUnicoding::registerPlugin((UnicodingInitFunc)h);
                LOGDBG("Plugins: Loaded %s\n", info.name.c_str());
            } else {
                LOGWRN("Failed to load %s: %s\n", info.name.c_str(), dlerror());
            }
            break;
        }
    }
}
