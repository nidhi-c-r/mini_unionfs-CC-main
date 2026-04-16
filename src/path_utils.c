
#define _GNU_SOURCE
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <limits.h>
#include <libgen.h>
#include <fuse3/fuse.h>
#include "unionfs.h"

struct mini_unionfs_state {
    char lower_dir[PATH_MAX];
    char upper_dir[PATH_MAX];
};

#define UNIONFS_DATA ((struct mini_unionfs_state *) fuse_get_context()->private_data)

// Build full path
void build_path(char *buf, const char *dir, const char *path) {
    if (path[0] == '/')
        snprintf(buf, PATH_MAX, "%s%s", dir, path);
    else
        snprintf(buf, PATH_MAX, "%s/%s", dir, path);
}

// Build whiteout path
void build_whiteout(char *buf, const char *dir, const char *path) {
    char d_path[PATH_MAX], b_path[PATH_MAX];
    strncpy(d_path, path, PATH_MAX - 1);
    strncpy(b_path, path, PATH_MAX - 1);

    char *d_name = dirname(d_path);
    char *b_name = basename(b_path);

    if (strcmp(d_name, "/") == 0 || strcmp(d_name, ".") == 0)
        snprintf(buf, PATH_MAX, "%s/.wh.%s", dir, b_name);
    else
        snprintf(buf, PATH_MAX, "%s/%s/.wh.%s", dir, d_name, b_name);
}

// Resolve file path
int resolve_path(const char *path, char *resolved_path) {
    struct mini_unionfs_state *st = UNIONFS_DATA;
    char upper[PATH_MAX], lower[PATH_MAX], whiteout[PATH_MAX];

    build_path(upper, st->upper_dir, path);
    build_path(lower, st->lower_dir, path);
    build_whiteout(whiteout, st->upper_dir, path);

    if (access(whiteout, F_OK) == 0) return -ENOENT;

    if (access(upper, F_OK) == 0) {
        strcpy(resolved_path, upper);
        return 0;
    }

    if (access(lower, F_OK) == 0) {
        strcpy(resolved_path, lower);
        return 0;
    }

    return -ENOENT;
}