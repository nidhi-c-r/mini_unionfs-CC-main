//part1(muskan)
#define _GNU_SOURCE
#define FUSE_USE_VERSION 31

#include <fuse3/fuse.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <sys/stat.h>
#include <dirent.h>
#include <fcntl.h>
#include <libgen.h>

struct mini_unionfs_state {
    char lower_dir[PATH_MAX];
    char upper_dir[PATH_MAX];
};

#define UNIONFS_DATA ((struct mini_unionfs_state *) fuse_get_context()->private_data)

// Helper to build paths without double slashes
static void build_path(char *buf, const char *dir, const char *path) {
    if (path[0] == '/') {
        snprintf(buf, PATH_MAX, "%s%s", dir, path);
    } else {
        snprintf(buf, PATH_MAX, "%s/%s", dir, path);
    }
}

// Robust Whiteout Builder
static void build_whiteout(char *buf, const char *dir, const char *path) {
    char d_path[PATH_MAX], b_path[PATH_MAX];
    strncpy(d_path, path, PATH_MAX - 1);
    strncpy(b_path, path, PATH_MAX - 1);
    d_path[PATH_MAX-1] = b_path[PATH_MAX-1] = '\0';

    char *d_name = dirname(d_path);
    char *b_name = basename(b_path);

    if (strcmp(d_name, "/") == 0 || strcmp(d_name, ".") == 0) {
        snprintf(buf, PATH_MAX, "%s/.wh.%s", dir, b_name);
    } else {
        snprintf(buf, PATH_MAX, "%s/%s/.wh.%s", dir, d_name, b_name);
    }
}

//part2(anish)

//part3(udupa)
static int unionfs_open(const char *path, struct fuse_file_info *fi) {
    char resolved[PATH_MAX];
    struct mini_unionfs_state *st = UNIONFS_DATA;
    if (resolve_path(path, resolved) != 0) return -ENOENT;

    if ((fi->flags & (O_WRONLY | O_RDWR))) {
        // If it's currently in lower, copy it to upper
        if (strncmp(resolved, st->lower_dir, strlen(st->lower_dir)) == 0) {
            if (copy_to_upper(path) != 0) return -EIO;
            build_path(resolved, st->upper_dir, path);
        }
    }
    int fd = open(resolved, fi->flags);
    if (fd < 0) return -errno;
    fi->fh = fd;
    return 0;
}

static int unionfs_read(const char *p, char *b, size_t s, off_t o, struct fuse_file_info *fi) {
    (void)p; int res = pread(fi->fh, b, s, o);
    return res < 0 ? -errno : res;
}

static int unionfs_write(const char *p, const char *b, size_t s, off_t o, struct fuse_file_info *fi) {
    (void)p; int res = pwrite(fi->fh, b, s, o);
    return res < 0 ? -errno : res;
}

static int unionfs_readdir(const char *path, void *buf, fuse_fill_dir_t filler, off_t offset, struct fuse_file_info *fi, enum fuse_readdir_flags flags) {
    (void)offset; (void)fi; (void)flags;
    struct mini_unionfs_state *st = UNIONFS_DATA;
    DIR *dp; struct dirent *de;
    char upper[PATH_MAX], lower[PATH_MAX];
    build_path(upper, st->upper_dir, path);
    build_path(lower, st->lower_dir, path);

    filler(buf, ".", NULL, 0, 0);
    filler(buf, "..", NULL, 0, 0);

    dp = opendir(upper);
    if (dp) {
        while ((de = readdir(dp)) != NULL) {
            if (!strcmp(de->d_name, ".") || !strcmp(de->d_name, "..")) continue;
            if (!strncmp(de->d_name, ".wh.", 4)) continue;
            filler(buf, de->d_name, NULL, 0, 0);
        }
        closedir(dp);
    }

    dp = opendir(lower);
    if (dp) {
        while ((de = readdir(dp)) != NULL) {
            if (!strcmp(de->d_name, ".") || !strcmp(de->d_name, "..")) continue;
            char wh_name[PATH_MAX], up_name[PATH_MAX];
            snprintf(wh_name, PATH_MAX, "%s/.wh.%s", upper, de->d_name);
            snprintf(up_name, PATH_MAX, "%s/%s", upper, de->d_name);
            if (access(wh_name, F_OK) == 0 || access(up_name, F_OK) == 0) continue;
            filler(buf, de->d_name, NULL, 0, 0);
        }
        closedir(dp);
    }
    return 0;
}
//part4(cr)
static int unionfs_unlink(const char *path) {
    struct mini_unionfs_state *st = UNIONFS_DATA;
    char upper[PATH_MAX], lower[PATH_MAX], whiteout[PATH_MAX];
    build_path(upper, st->upper_dir, path);
    build_path(lower, st->lower_dir, path);
    build_whiteout(whiteout, st->upper_dir, path);

    int in_upper = (access(upper, F_OK) == 0);
    int in_lower = (access(lower, F_OK) == 0);

    if (in_upper) {
        if (unlink(upper) < 0) return -errno;
    }
    if (in_lower) {
        // Create the whiteout file
        int fd = open(whiteout, O_CREAT | O_WRONLY, 0644);
        if (fd < 0) return -errno;
        close(fd);
    }
    
    if (!in_upper && !in_lower) return -ENOENT;
    return 0;
}

static int unionfs_release(const char *p, struct fuse_file_info *fi) {
    (void)p; if (fi->fh > 0) close(fi->fh); return 0;
}

static int unionfs_mkdir(const char *path, mode_t mode) {
    struct mini_unionfs_state *st = UNIONFS_DATA;
    char upper[PATH_MAX];
    build_path(upper, st->upper_dir, path);
    return mkdir(upper, mode) == -1 ? -errno : 0;
}

static struct fuse_operations unionfs_oper = {
    .getattr = unionfs_getattr, .readdir = unionfs_readdir, .open = unionfs_open,
    .read = unionfs_read, .write = unionfs_write, .unlink = unionfs_unlink,
    .release = unionfs_release, .mkdir = unionfs_mkdir,
};


int main(int argc, char *argv[]) {
    if (argc < 4) return 1;
    struct mini_unionfs_state *state = malloc(sizeof(struct mini_unionfs_state));
    if (!realpath(argv[1], state->lower_dir) || !realpath(argv[2], state->upper_dir)) return 1;
    argv[1] = argv[3]; argc = 2;
    return fuse_main(argc, argv, &unionfs_oper, state);
}