#ifndef UNIONFS_H
#define UNIONFS_H

#include <limits.h>

struct mini_unionfs_state {
    char lower_dir[PATH_MAX];
    char upper_dir[PATH_MAX];
};

void build_path(char*, const char*, const char*);
void build_whiteout(char*, const char*, const char*);
int resolve_path(const char*, char*);

#endif