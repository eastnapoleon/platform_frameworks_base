/*
 * Copyright (C) 2010 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include <utils/ObbFile.h>
#include <utils/String8.h>

#include <getopt.h>
#include <stdio.h>
#include <stdlib.h>

using namespace android;

static const char* gProgName = "obbtool";
static const char* gProgVersion = "1.0";

static int wantUsage = 0;
static int wantVersion = 0;

#define ADD_OPTS "n:v:f:c:"
static const struct option longopts[] = {
    {"help",       no_argument, &wantUsage,   1},
    {"version",    no_argument, &wantVersion, 1},

    /* Args for "add" */
    {"name",       required_argument, NULL, 'n'},
    {"version",    required_argument, NULL, 'v'},
    {"filesystem", required_argument, NULL, 'f'},
    {"crypto",     required_argument, NULL, 'c'},

    {NULL, 0, NULL, '\0'}
};

struct package_info_t {
    char* packageName;
    int packageVersion;
    char* filesystem;
    char* crypto;
};

/*
 * Print usage info.
 */
void usage(void)
{
    fprintf(stderr, "Opaque Binary Blob (OBB) Tool\n\n");
    fprintf(stderr, "Usage:\n");
    fprintf(stderr,
        " %s a[dd] [ OPTIONS ] FILENAME\n"
        "   Adds an OBB signature to the file.\n\n", gProgName);
    fprintf(stderr,
        " %s r[emove] FILENAME\n"
        "   Removes the OBB signature from the file.\n\n", gProgName);
    fprintf(stderr,
        " %s i[nfo] FILENAME\n"
        "   Prints the OBB signature information of a file.\n\n", gProgName);
}

void doAdd(const char* filename, struct package_info_t* info) {
    ObbFile *obb = new ObbFile();
    if (obb->readFrom(filename)) {
        fprintf(stderr, "ERROR: %s: OBB signature already present\n", filename);
        return;
    }

    obb->setPackageName(String8(info->packageName));
    obb->setVersion(info->packageVersion);

    if (!obb->writeTo(filename)) {
        fprintf(stderr, "ERROR: %s: couldn't write OBB signature: %s\n",
                filename, strerror(errno));
        return;
    }

    fprintf(stderr, "OBB signature successfully written\n");
}

void doRemove(const char* filename) {
    ObbFile *obb = new ObbFile();
    if (!obb->readFrom(filename)) {
        fprintf(stderr, "ERROR: %s: no OBB signature present\n", filename);
        return;
    }

    if (!obb->removeFrom(filename)) {
        fprintf(stderr, "ERROR: %s: couldn't remove OBB signature\n", filename);
        return;
    }

    fprintf(stderr, "OBB signature successfully removed\n");
}

void doInfo(const char* filename) {
    ObbFile *obb = new ObbFile();
    if (!obb->readFrom(filename)) {
        fprintf(stderr, "ERROR: %s: couldn't read OBB signature\n", filename);
        return;
    }

    printf("OBB info for '%s':\n", filename);
    printf("Package name: %s\n", obb->getPackageName().string());
    printf("     Version: %d\n", obb->getVersion());
}

/*
 * Parse args.
 */
int main(int argc, char* const argv[])
{
    const char *prog = argv[0];
    struct options *options;
    int opt;
    int option_index = 0;
    struct package_info_t package_info;

    int result = 1;    // pessimistically assume an error.

    if (argc < 2) {
        wantUsage = 1;
        goto bail;
    }

    while ((opt = getopt_long(argc, argv, ADD_OPTS, longopts, &option_index)) != -1) {
        switch (opt) {
        case 0:
            if (longopts[option_index].flag)
                break;
            fprintf(stderr, "'%s' requires an argument\n", longopts[option_index].name);
            wantUsage = 1;
            goto bail;
        case 'n':
            package_info.packageName = optarg;
            break;
        case 'v':
            char *end;
            package_info.packageVersion = strtol(optarg, &end, 10);
            if (*optarg == '\0' || *end != '\0') {
                fprintf(stderr, "ERROR: invalid version; should be integer!\n\n");
                wantUsage = 1;
                goto bail;
            }
            break;
        case 'f':
            package_info.filesystem = optarg;
            break;
        case 'c':
            package_info.crypto = optarg;
            break;
        case '?':
            wantUsage = 1;
            goto bail;
        }
    }

    if (wantVersion) {
        fprintf(stderr, "%s %s\n", gProgName, gProgVersion);
    }

    if (wantUsage) {
        goto bail;
    }

#define CHECK_OP(name) \
    if (strncmp(op, name, opsize)) { \
        fprintf(stderr, "ERROR: unknown function '%s'!\n\n", op); \
        wantUsage = 1; \
        goto bail; \
    }

    if (optind < argc) {
        const char* op = argv[optind++];
        const int opsize = strlen(op);

        if (optind >= argc) {
            fprintf(stderr, "ERROR: filename required!\n\n");
            wantUsage = 1;
            goto bail;
        }

        const char* filename = argv[optind++];

        switch (op[0]) {
        case 'a':
            CHECK_OP("add");
            if (package_info.packageName == NULL) {
                fprintf(stderr, "ERROR: arguments required 'packageName' and 'version'\n");
                goto bail;
            }
            doAdd(filename, &package_info);
            break;
        case 'r':
            CHECK_OP("remove");
            doRemove(filename);
            break;
        case 'i':
            CHECK_OP("info");
            doInfo(filename);
            break;
        default:
            fprintf(stderr, "ERROR: unknown command '%s'!\n\n", op);
            wantUsage = 1;
            goto bail;
        }
    }

bail:
    if (wantUsage) {
        usage();
        result = 2;
    }

    return result;
}