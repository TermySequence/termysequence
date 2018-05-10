// Copyright © 2018 TermySequence LLC
//
// SPDX-License-Identifier: GPL-2.0-only

#include <libxml/parser.h>
#include <libxml/xpath.h>
#include <sys/types.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <dirent.h>
#include <limits.h>

#define QUERY "//context[name[text()='server']]/message" \
    "[translation[@type='finished' or not(@type)]]"

static void
usage()
{
    fputs("Usage: extract <path-to-toplevel-git-folder>\n", stderr);
    exit(1);
}

static int
isTsFile(const char *name, const char *dirpath, char **files)
{
    char buf[PATH_MAX];
    size_t len = strlen(name);

    if (len > 3 && !strcmp(name + len - 3, ".ts")) {
        strcpy(buf, dirpath);
        strcat(buf, "/src/i18n/");
        strcat(buf, name);
        files[0] = strdup(buf);

        strcpy(buf, dirpath);
        strcat(buf, "/mux/i18n/");
        strcat(buf, name);

        buf[strlen(buf) - 2] = '\0';
        strcat(buf, "txt");
        files[1] = strdup(buf);

        return 1;
    }
    return 0;
}

static void
handleTsFile(const char *inpath, const char *outpath)
{
    xmlDocPtr doc;
    xmlXPathContextPtr context;
    xmlXPathObjectPtr result;
    xmlNodeSetPtr nodeset;
    xmlNodePtr cur;
    xmlChar *query = (xmlChar*)QUERY;
    xmlChar *key, *val;
    const char *inname = rindex(inpath, '/') + 1;
    const char *outname = rindex(outpath, '/') + 1;
    FILE *file;
    int i, count;

    doc = xmlParseFile(inpath);
    if (!doc) {
        fprintf(stderr, "Failed to parse %s\n", inname);
        return;
    }

    context = xmlXPathNewContext(doc);
    if (!context)
        goto err;

    result = xmlXPathEvalExpression(query, context);
    if (!result)
        goto err;

    nodeset = result->nodesetval;
    if (xmlXPathNodeSetIsEmpty(nodeset)) {
        fprintf(stderr, "No server translations found in %s\n", inname);
        goto out;
    }

    file = fopen(outpath, "w");
    for (i = 0, count = 0; i < nodeset->nodeNr; ++i) {
        key = NULL;
        val = NULL;
        for (cur = nodeset->nodeTab[i]->xmlChildrenNode; cur; cur = cur->next) {
            if (!xmlStrcmp(cur->name, (const xmlChar*)"comment")) {
                key = xmlNodeListGetString(doc, cur->xmlChildrenNode, 1);
                break;
            }
        }
        for (cur = nodeset->nodeTab[i]->xmlChildrenNode; cur; cur = cur->next) {
            if (!xmlStrcmp(cur->name, (const xmlChar*)"translation")) {
                val = xmlNodeListGetString(doc, cur->xmlChildrenNode, 1);
                break;
            }
        }

        if (key && *key && val && *val) {
            fprintf(file, "%s=%s\n", key, val);
            ++count;
        }

        xmlFree(key);
        xmlFree(val);
    }

    fclose(file);
    if (count) {
        printf("Extracted %d strings from %s to %s\n", count, inname, outname);
    } else {
        printf("Extracted no strings from %s\n", inname);
        unlink(outpath);
    }
out:
    xmlXPathFreeObject(result);
    xmlFreeDoc(doc);
    return;
err:
    xmlFreeDoc(doc);
    fprintf(stderr, "Error processing %s\n", inname);
}

int
main(int argc, char **argv)
{
    DIR *dir;
    struct dirent *ent;
    char **files;
    size_t i, nfiles;

    if (argc != 2)
        usage();

    if (chdir(argv[1]) != 0) {
        fprintf(stderr, "%s: %m\n", argv[1]);
        usage();
    }

    dir = opendir("src/i18n");
    if (!dir) {
        fprintf(stderr, "%s/src/i18n: %m\n", argv[1]);
        return 2;
    }

    nfiles = 0;
    while ((ent = readdir(dir))) {
        ++nfiles;
    }

    files = (char**)calloc(nfiles * 2 + 1, sizeof(char*));
    rewinddir(dir);

    i = 0;
    while ((ent = readdir(dir)))
        if (ent->d_type == DT_REG && isTsFile(ent->d_name, argv[1], files + i))
            i += 2;

    i = 0;
    while (files[i]) {
        handleTsFile(files[i], files[i + 1]);
        i += 2;
    }

    printf("Processed %zu files\n", i / 2);
    return 0;
}
