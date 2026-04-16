#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <limits.h>
#include <fuse3/fuse.h>
#include "unionfs.h"

extern void build_path(char*, const char*, const char*);
extern int resolve_path(const char*, char*);

#define UNIONFS_DATA ((struct mini_unionfs_state *) fuse_get_context()->private_data)

int copy_to_upper(const char *path) {
    struct mini_unionfs_state *st = UNIONFS_DATA;
    char lower[PATH_MAX], upper[PATH_MAX];

    build_path(lower, st->lower_dir, path);
    build_path(upper, st->upper_dir, path);

    int src = open(lower, O_RDONLY);
    int dst = open(upper, O_WRONLY | O_CREAT | O_TRUNC, 0644);

    char buf[4096];
    ssize_t r;
    while ((r = read(src, buf, sizeof(buf))) > 0)
        write(dst, buf, r);

    close(src);
    close(dst);
    return 0;
}

int unionfs_open(const char *path, struct fuse_file_info *fi) {
    char resolved[PATH_MAX];
    struct mini_unionfs_state *st = UNIONFS_DATA;

    if (resolve_path(path, resolved) != 0) return -ENOENT;

    if ((fi->flags & (O_WRONLY | O_RDWR))) {
        if (strstr(resolved, st->lower_dir)) {
            copy_to_upper(path);
            build_path(resolved, st->upper_dir, path);
        }
    }

    int fd = open(resolved, fi->flags);
    fi->fh = fd;
    return 0;
}

int unionfs_read(const char *p, char *b, size_t s, off_t o, struct fuse_file_info *fi) {
    return pread(fi->fh, b, s, o);
}

int unionfs_write(const char *p, const char *b, size_t s, off_t o, struct fuse_file_info *fi) {
    return pwrite(fi->fh, b, s, o);
}