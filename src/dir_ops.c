#include <dirent.h>
#include <string.h>
#include <limits.h>
#include <unistd.h>
#include <fuse3/fuse.h>
#include "unionfs.h"

extern void build_path(char*, const char*, const char*);
extern int resolve_path(const char*, char*);

#define UNIONFS_DATA ((struct mini_unionfs_state *) fuse_get_context()->private_data)

int unionfs_getattr(const char *path, struct stat *stbuf, struct fuse_file_info *fi) {
    char resolved[PATH_MAX];
    if (resolve_path(path, resolved) != 0) return -ENOENT;
    return lstat(resolved, stbuf);
}

int unionfs_readdir(const char *path, void *buf, fuse_fill_dir_t filler,
                   off_t offset, struct fuse_file_info *fi,
                   enum fuse_readdir_flags flags) {

    struct mini_unionfs_state *st = UNIONFS_DATA;
    char upper[PATH_MAX], lower[PATH_MAX];

    build_path(upper, st->upper_dir, path);
    build_path(lower, st->lower_dir, path);

    DIR *dp = opendir(upper);
    struct dirent *de;

    while (dp && (de = readdir(dp)) != NULL)
        filler(buf, de->d_name, NULL, 0, 0);

    closedir(dp);

    dp = opendir(lower);
    while (dp && (de = readdir(dp)) != NULL)
        filler(buf, de->d_name, NULL, 0, 0);

    closedir(dp);

    return 0;
}
