/*
  FUSE: Filesystem in Userspace
  Copyright (C) 2001-2007  Miklos Szeredi <miklos@szeredi.hu>
  Copyright (C) 2011       Sebastian Pipping <sebastian@pipping.org>

  This program can be distributed under the terms of the GNU GPL.
  See the file COPYING.

  gcc -Wall fusexmp.c `pkg-config fuse --cflags --libs` -o fusexmp
*/

#define FUSE_USE_VERSION 26

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif
#include "params.h"

#ifdef linux
/* For pread()/pwrite()/utimensat() */
#define _XOPEN_SOURCE 700
#endif

#include <fuse.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <dirent.h>
#include <errno.h>
#include <sys/time.h>
#include <stdlib.h>
#ifdef HAVE_SETXATTR
#include <sys/xattr.h>
#endif


//  All the paths I see are relative to the root of the mounted
//  filesystem.  In order to get to the underlying filesystem, I need to
//  have the mountpoint.  I'll save it away early on in main(), and then
//  whenever I need a path for something I'll call this to construct
//  it.
static void bb_fullpath(char fpath[PATH_MAX], const char *path)
{
    printf("PATH_MAX = %d\n", PATH_MAX);
    if (BB_DATA != NULL)
    {
        printf("rootDir:%s\n", BB_DATA->rootdir);
    }
    else
    {
        printf("BB_DATA is null\n");
    }
    fflush(stdout);

    strcpy(fpath, BB_DATA->rootdir);
    strncat(fpath, path, PATH_MAX); // ridiculously long paths will
                                    // break here

    printf("    bb_fullpath:  rootdir = \"%s\", path = \"%s\", fpath = \"%s\"\n",
            BB_DATA->rootdir, path, fpath);
}

static int xmp_getattr(const char *path, struct stat *stbuf)
{
    char fpath[PATH_MAX];
    bb_fullpath(fpath, path);
    printf("xmp_getattr: path = %s\n", fpath);
    fflush(stdout);
    int res;

    res = lstat(fpath, stbuf);
    if (res == -1)
            return -errno;

    return 0;
}

static int xmp_access(const char *path, int mask)
{
    char fpath[PATH_MAX];
    int res;
    bb_fullpath(fpath, path);

    printf("xmp_access: path = %s\n", fpath);
    fflush(stdout);
    res = access(fpath, mask);
    if (res == -1)
            return -errno;

    return 0;
}

static int xmp_readlink(const char *path, char *buf, size_t size)
{
    char fpath[PATH_MAX];
    int res;
    bb_fullpath(fpath, path);
    printf("xmp_readlink: path = %s\n", fpath);
    fflush(stdout);
    res = readlink(fpath, buf, size - 1);
    if (res == -1)
            return -errno;

    buf[res] = '\0';
    return 0;
}


static int xmp_readdir(const char *path, void *buf, fuse_fill_dir_t filler,
		       off_t offset, struct fuse_file_info *fi)
{
    char fpath[PATH_MAX];
    bb_fullpath(fpath, path);
    printf("xmp_readdir: path = %s\n", fpath);
    fflush(stdout);
    DIR *dp;
    struct dirent *de;

    (void) offset;
    (void) fi;

    dp = opendir(fpath);
    if (dp == NULL)
            return -errno;

    while ((de = readdir(dp)) != NULL) {
            struct stat st;
            memset(&st, 0, sizeof(st));
            st.st_ino = de->d_ino;
            st.st_mode = de->d_type << 12;
            if (filler(buf, de->d_name, &st, 0))
                    break;
    }

    closedir(dp);
    return 0;
}

static int xmp_mknod(const char *path, mode_t mode, dev_t rdev)
{
    char fpath[PATH_MAX];
    int res;
    bb_fullpath(fpath, path);
    printf("xmp_mknod: path = %s\n", fpath);
    fflush(stdout);

    /* On Linux this could just be 'mknod(path, mode, rdev)' but this
       is more portable */
    if (S_ISREG(mode)) {
            res = open(fpath, O_CREAT | O_EXCL | O_WRONLY, mode);
            if (res >= 0)
                    res = close(res);
    } else if (S_ISFIFO(mode))
            res = mkfifo(fpath, mode);
    else
            res = mknod(fpath, mode, rdev);
    if (res == -1)
            return -errno;

    return 0;
}

static int xmp_mkdir(const char *path, mode_t mode)
{
    char fpath[PATH_MAX];
    bb_fullpath(fpath, path);
    printf("xmp_mkdir: path = %s\n", fpath);
    fflush(stdout);
    int res;

    res = mkdir(fpath, mode);
    if (res == -1)
            return -errno;

    return 0;
}

static int xmp_unlink(const char *path)
{
    char fpath[PATH_MAX];
    bb_fullpath(fpath, path);
    printf("xmp_unlink: path = %s\n", fpath);
    fflush(stdout);
    int res;

    res = unlink(fpath);
    if (res == -1)
            return -errno;

    return 0;
}

static int xmp_rmdir(const char *path)
{
    char fpath[PATH_MAX];
    int res;
    bb_fullpath(fpath, path);
    res = rmdir(fpath);
    if (res == -1)
            return -errno;

    return 0;
}

static int xmp_symlink(const char *from, const char *to)
{
    int res;

    res = symlink(from, to);
    if (res == -1)
            return -errno;

    return 0;
}

static int xmp_rename(const char *from, const char *to)
{
    int res;

    res = rename(from, to);
    if (res == -1)
            return -errno;

    return 0;
}

static int xmp_link(const char *from, const char *to)
{
    int res;

    res = link(from, to);
    if (res == -1)
            return -errno;

    return 0;
}

static int xmp_chmod(const char *path, mode_t mode)
{
    int res;

    res = chmod(path, mode);
    if (res == -1)
            return -errno;

    return 0;
}

static int xmp_chown(const char *path, uid_t uid, gid_t gid)
{
    int res;

    res = lchown(path, uid, gid);
    if (res == -1)
            return -errno;

    return 0;
}

static int xmp_truncate(const char *path, off_t size)
{
    char fpath[PATH_MAX];
    bb_fullpath(fpath, path);
    printf("xmp_truncate: path = %s\n", fpath);
    fflush(stdout);
    int res;

    res = truncate(fpath, size);
    if (res == -1)
            return -errno;

    return 0;
}

#ifdef HAVE_UTIMENSAT
static int xmp_utimens(const char *path, const struct timespec ts[2])
{
    printf("xmp_utimens: path = %s\n", path);
    fflush(stdout);
    int res;

    /* don't use utime/utimes since they follow symlinks */
    res = utimensat(0, path, ts, AT_SYMLINK_NOFOLLOW);
    if (res == -1)
            return -errno;

    return 0;
}
#endif

static int xmp_open(const char *path, struct fuse_file_info *fi)
{
    char fpath[PATH_MAX];
    bb_fullpath(fpath, path);
    printf("xmp_open: path = %s\n", fpath);
    fflush(stdout);
    int res;

    res = open(fpath, fi->flags);
    if (res == -1)
            return -errno;

    close(res);
    return 0;
}

static int xmp_read(const char *path, char *buf, size_t size, off_t offset,
		    struct fuse_file_info *fi)
{
    char fpath[PATH_MAX];
    bb_fullpath(fpath, path);
    printf("xmp_read: path = %s\n", fpath);
    fflush(stdout);
    int fd;
    int res;

    (void) fi;
    fd = open(fpath, O_RDONLY);
    if (fd == -1)
            return -errno;

    res = pread(fd, buf, size, offset);
    if (res == -1)
            res = -errno;

    close(fd);
    return res;
}

static int xmp_write(const char *path, const char *buf, size_t size,
		     off_t offset, struct fuse_file_info *fi)
{
    char fpath[PATH_MAX];
    bb_fullpath(fpath, path);
    printf("xmp_write: path = %s\n", fpath);
    fflush(stdout);
    int fd;
    int res;

    (void) fi;
    fd = open(fpath, O_WRONLY);
    if (fd == -1)
            return -errno;

    res = pwrite(fd, buf, size, offset);
    if (res == -1)
            res = -errno;

    close(fd);
    printf("Writing using our xmp_write\n");
    fflush(stdout);
    return res;
}

static int xmp_statfs(const char *path, struct statvfs *stbuf)
{
    char fpath[PATH_MAX];
    bb_fullpath(fpath, path);
    printf("xmp_statfs: path = %s\n", fpath);
    fflush(stdout);
    int res;

    res = statvfs(fpath, stbuf);
    if (res == -1)
            return -errno;

    return 0;
}

static int xmp_release(const char *path, struct fuse_file_info *fi)
{
    char fpath[PATH_MAX];
    bb_fullpath(fpath, path);
    printf("xmp_release: path = %s\n", fpath);
    fflush(stdout);
    /* Just a stub.	 This method is optional and can safely be left
       unimplemented */

    (void) path;
    (void) fi;
    return 0;
}

static int xmp_fsync(const char *path, int isdatasync,
		     struct fuse_file_info *fi)
{
    /* Just a stub.	 This method is optional and can safely be left
       unimplemented */

    (void) path;
    (void) isdatasync;
    (void) fi;
    return 0;
}

#ifdef HAVE_POSIX_FALLOCATE
static int xmp_fallocate(const char *path, int mode,
			off_t offset, off_t length, struct fuse_file_info *fi)
{
    printf("xmp_fallocate: path = %s\n", path);
    fflush(stdout);
    int fd;
    int res;

    (void) fi;

    if (mode)
            return -EOPNOTSUPP;

    fd = open(path, O_WRONLY);
    if (fd == -1)
            return -errno;

    res = -posix_fallocate(fd, offset, length);

    close(fd);
    return res;
}
#endif

#ifdef HAVE_SETXATTR
/* xattr operations are optional and can safely be left unimplemented */
static int xmp_setxattr(const char *path, const char *name, const char *value,
			size_t size, int flags)
{
    char fpath[PATH_MAX];
    bb_fullpath(fpath, path);
    printf("xmp_setxattr: path = %s\n", fpath);
    fflush(stdout);
    int res = lsetxattr(fpath, name, value, size, flags);
    if (res == -1)
            return -errno;
    return 0;
}

static int xmp_getxattr(const char *path, const char *name, char *value,
			size_t size)
{
    char fpath[PATH_MAX];
    bb_fullpath(fpath, path);
    printf("xmp_getxattr: path = %s\n", fpath);
    fflush(stdout);
    int res = lgetxattr(fpath, name, value, size);
    if (res == -1)
            return -errno;
    return res;
}

static int xmp_listxattr(const char *path, char *list, size_t size)
{
    char fpath[PATH_MAX];
    bb_fullpath(fpath, path);
    printf("xmp_listxattr: path = %s\n", fpath);
    fflush(stdout);
    int res = llistxattr(fpath, list, size);
    if (res == -1)
            return -errno;
    return res;
}

static int xmp_removexattr(const char *path, const char *name)
{
    char fpath[PATH_MAX];
    bb_fullpath(fpath, path);
    printf("xmp_removexattr: path = %s\n", fpath);
    fflush(stdout);
    int res = lremovexattr(fpath, name);
    if (res == -1)
            return -errno;
    return 0;
}
#endif /* HAVE_SETXATTR */

static struct fuse_operations xmp_oper = {
    .getattr	= xmp_getattr,
    .access		= xmp_access,
    .readlink	= xmp_readlink,
    .readdir	= xmp_readdir,
    .mknod		= xmp_mknod,
    .mkdir		= xmp_mkdir,
    .symlink	= xmp_symlink,
    .unlink		= xmp_unlink,
    .rmdir		= xmp_rmdir,
    .rename		= xmp_rename,
    .link		= xmp_link,
    .chmod		= xmp_chmod,
    .chown		= xmp_chown,
    .truncate	= xmp_truncate,
#ifdef HAVE_UTIMENSAT
    .utimens	= xmp_utimens,
#endif
    .open		= xmp_open,
    .read		= xmp_read,
    .write		= xmp_write,
    .statfs		= xmp_statfs,
    .release	= xmp_release,
    .fsync		= xmp_fsync,
#ifdef HAVE_POSIX_FALLOCATE
    .fallocate	= xmp_fallocate,
#endif
#ifdef HAVE_SETXATTR
    .setxattr	= xmp_setxattr,
    .getxattr	= xmp_getxattr,
    .listxattr	= xmp_listxattr,
    .removexattr	= xmp_removexattr,
#endif
};

void bb_usage()
{
    fprintf(stderr, "usage:  fusexmp [FUSE and mount options] rootDir mountPoint\n");
    //abort();
}

int main(int argc, char *argv[])
{
    struct bb_state *bb_data;
    umask(0);
    // Perform some sanity checking on the command line:  make sure
    // there are enough arguments, and that neither of the last two
    // start with a hyphen (this will break if you actually have a
    // rootpoint or mountpoint whose name starts with a hyphen, but so
    // will a zillion other programs)
    //if ((argc < 3) || (argv[argc-2][0] == '-') || (argv[argc-1][0] == '-'))
    //    bb_usage();

    bb_usage();

    bb_data = malloc(sizeof(struct bb_state));
    if (bb_data == NULL) {
        perror("main calloc");
        abort();
    }

    // Pull the rootdir out of the argument list and save it in my
    // internal data

    bb_data->rootdir = realpath(argv[argc-2], NULL);
    fflush(stdout);
    argv[argc-2] = argv[argc-1];
    argv[argc-1] = NULL;
    argc--;

    return fuse_main(argc, argv, &xmp_oper, bb_data);
}
