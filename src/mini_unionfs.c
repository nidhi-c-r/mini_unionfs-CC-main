//part1(muskan)

//part2(anish)

//part3(udupa)

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