#include <fuse3/fuse.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <limits.h>

extern void build_path(char*, const char*, const char*);
extern void build_whiteout(char*, const char*, const char*);

#define UNIONFS_DATA ((struct mini_unionfs_state *) fuse_get_context()->private_data)

int unionfs_unlink(const char *path) {
    struct mini_unionfs_state *st = UNIONFS_DATA;

    char upper[PATH_MAX], lower[PATH_MAX], whiteout[PATH_MAX];

    build_path(upper, st->upper_dir, path);
    build_path(lower, st->lower_dir, path);
    build_whiteout(whiteout, st->upper_dir, path);

    unlink(upper);

    int fd = open(whiteout, O_CREAT, 0644);
    close(fd);

    return 0;
}

int unionfs_release(const char *p, struct fuse_file_info *fi) {
    close(fi->fh);
    return 0;
}

int unionfs_mkdir(const char *path, mode_t mode) {
    struct mini_unionfs_state *st = UNIONFS_DATA;
    char upper[PATH_MAX];
    build_path(upper, st->upper_dir, path);
    return mkdir(upper, mode);
}

extern int unionfs_getattr();
extern int unionfs_readdir();
extern int unionfs_open();
extern int unionfs_read();
extern int unionfs_write();

static struct fuse_operations ops = {
    .getattr = unionfs_getattr,
    .readdir = unionfs_readdir,
    .open = unionfs_open,
    .read = unionfs_read,
    .write = unionfs_write,
    .unlink = unionfs_unlink,
    .release = unionfs_release,
    .mkdir = unionfs_mkdir,
};

int main(int argc, char *argv[]) {
    struct mini_unionfs_state *state = malloc(sizeof(*state));

    realpath(argv[1], state->lower_dir);
    realpath(argv[2], state->upper_dir);

    argv[1] = argv[3];
    argc = 2;

    return fuse_main(argc, argv, &ops, state);
}