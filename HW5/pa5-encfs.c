/*
  FUSE: Filesystem in Userspace
  Copyright (C) 2001-2007  Miklos Szeredi <miklos@szeredi.hu>

  Minor modifications and note by Andy Sayler (2012) <www.andysayler.com>

  Source: fuse-2.8.7.tar.gz examples directory
  http://sourceforge.net/projects/fuse/files/fuse-2.X/

  This program can be distributed under the terms of the GNU GPL.
  See the file COPYING.

  gcc -Wall `pkg-config fuse --cflags` fusexmp.c -o fusexmp `pkg-config fuse --libs`

  Note: This implementation is largely stateless and does not maintain
        open file handels between open and release calls (fi->fh).
        Instead, files are opened and closed as necessary inside read(), write(),
        etc calls. As such, the functions that rely on maintaining file handles are
        not implmented (fgetattr(), etc). Those seeking a more efficient and
        more complete implementation may wish to add fi->fh support to minimize
        open() and close() calls and support fh dependent functions.

*/

#define FUSE_USE_VERSION 28
#define HAVE_SETXATTR

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#ifdef linux
/* For pread()/pwrite() */
#define _XOPEN_SOURCE 500
#endif

#include <fuse.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <dirent.h>
#include <errno.h>
#include <sys/time.h>
#include <stdlib.h>
#ifdef HAVE_SETXATTR
#include <sys/xattr.h>
#endif

char debug = 0;
char * root_dir = NULL;

char* absolute_path(const char* original_path) {
	if (debug) {
		printf("Entering absolute_path\n");
	}
	
	size_t len = strlen(original_path) + strlen(root_dir) + 1;
	char * root_path = malloc(len*sizeof(char));
	
	strcpy(root_path, root_dir);
	strcat(root_path, original_path);
	
	return root_path;
}

static int pa5_encfs_getattr(const char *path, struct stat *stbuf)
{
	
	if (debug) {
		printf("Entering pa5_encfs_getattr\n");
	}
	
	int res;
	
	char* full_path = absolute_path(path);
	
	res = lstat(full_path, stbuf);
	if (res == -1)
		return -errno;

	return 0;
}

static int pa5_encfs_access(const char *path, int mask)
{
	if (debug) {
		printf("Entering pa5_encfs_access\n");
	}
	
	int res;
	
	char* full_path = absolute_path(path);

	res = access(full_path, mask);
	if (res == -1)
		return -errno;

	return 0;
}

static int pa5_encfs_readlink(const char *path, char *buf, size_t size)
{
	if (debug) {
		printf("Entering pa5_encfs_readlink\n");
	}	
	int res;
	
	char* full_path = absolute_path(path);

	res = readlink(full_path, buf, size - 1);
	if (res == -1)
		return -errno;

	buf[res] = '\0';
	return 0;
}


static int pa5_encfs_readdir(const char *path, void *buf, fuse_fill_dir_t filler,
		       off_t offset, struct fuse_file_info *fi)
{
	if (debug) {
		printf("Entering pa5_encfs_readdir\n");
	}
	
	char * full_path = absolute_path(path);
	printf("full_path = %s\n", full_path);
	
	DIR *dp;
	struct dirent *de;

	(void) offset;
	(void) fi;

	//dp = opendir(path);
	dp = opendir(full_path);
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

static int pa5_encfs_mknod(const char *path, mode_t mode, dev_t rdev)
{
	if (debug) {
		printf("Entering pa5_encfs_mknod\n");
	}
	
	char* full_path = absolute_path(path);
		
	int res;

	/* On Linux this could just be 'mknod(path, mode, rdev)' but this
	   is more portable */
	if (S_ISREG(mode)) {
		res = open(full_path, O_CREAT | O_EXCL | O_WRONLY, mode);
		if (res >= 0)
			res = close(res);
	} else if (S_ISFIFO(mode))
		res = mkfifo(full_path, mode);
	else
		res = mknod(full_path, mode, rdev);
	if (res == -1)
		return -errno;

	return 0;
}

static int pa5_encfs_mkdir(const char *path, mode_t mode)
{
	if (debug) {
		printf("Entering pa5_encfs_mkdir\n");
	}
	
	char* full_path = absolute_path(path);
	
	int res;

	res = mkdir(full_path, mode);
	if (res == -1)
		return -errno;

	return 0;
}

static int pa5_encfs_unlink(const char *path)
{
	if (debug) {
		printf("Entering pa5_encfs_unlink\n");
	}
	
	char* full_path = absolute_path(path);
	
	int res;

	res = unlink(full_path);
	if (res == -1)
		return -errno;

	return 0;
}

static int pa5_encfs_rmdir(const char *path)
{
	if (debug) {
		printf("Entering pa5_encfs_rmdir\n");
	}	
	
	char* full_path = absolute_path(path);
	
	int res;

	res = rmdir(full_path);
	if (res == -1)
		return -errno;

	return 0;
}

static int pa5_encfs_symlink(const char *from, const char *to)
{
	if (debug) {
		printf("Entering pa5_encfs_symlink\n");
	}	
	
	int res;

	res = symlink(from, to);
	if (res == -1)
		return -errno;

	return 0;
}

static int pa5_encfs_rename(const char *from, const char *to)
{
	if (debug) {
		printf("Entering pa5_encfs_rename\n");
	}	
	
	int res;

	res = rename(from, to);
	if (res == -1)
		return -errno;

	return 0;
}

static int pa5_encfs_link(const char *from, const char *to)
{
	if (debug) {
		printf("Entering pa5_encfs_link\n");
	}	
		
	int res;

	res = link(from, to);
	if (res == -1)
		return -errno;

	return 0;
}

static int pa5_encfs_chmod(const char *path, mode_t mode)
{
	if (debug) {
		printf("Entering pa5_encfs_chmod\n");
	}
		
	char* full_path = absolute_path(path);
	
	int res;

	res = chmod(full_path, mode);
	if (res == -1)
		return -errno;

	return 0;
}

static int pa5_encfs_chown(const char *path, uid_t uid, gid_t gid)
{
	if (debug) {
		printf("Entering pa5_encfs_chown\n");
	}
	
	int res;
	
	char* full_path = absolute_path(path);

	res = lchown(full_path, uid, gid);
	if (res == -1)
		return -errno;

	return 0;
}

static int pa5_encfs_truncate(const char *path, off_t size)
{
	if (debug) {
		printf("Entering pa5_encfs_truncate\n");
	}
		
	int res;
	
	char* full_path = absolute_path(path);

	res = truncate(full_path, size);
	if (res == -1)
		return -errno;

	return 0;
}

static int pa5_encfs_utimens(const char *path, const struct timespec ts[2])
{
	if (debug) {
		printf("Entering pa5_encfs_utimens\n");
	}
		
	int res;
	struct timeval tv[2];
	char* full_path = absolute_path(path);

	tv[0].tv_sec = ts[0].tv_sec;
	tv[0].tv_usec = ts[0].tv_nsec / 1000;
	tv[1].tv_sec = ts[1].tv_sec;
	tv[1].tv_usec = ts[1].tv_nsec / 1000;

	res = utimes(full_path, tv);
	if (res == -1)
		return -errno;

	return 0;
}

static int pa5_encfs_open(const char *path, struct fuse_file_info *fi)
{
	if (debug) {
		printf("Entering pa5_encfs_open\n");
	}
	
	int res;
	
	char* full_path = absolute_path(path);

	res = open(full_path, fi->flags);
	if (res == -1)
		return -errno;

	close(res);
	return 0;
}

static int pa5_encfs_read(const char *path, char *buf, size_t size, off_t offset,
		    struct fuse_file_info *fi)
{
	if (debug) {
		printf("Entering pa5_encfs_read\n");
	}
	
	char* full_path = absolute_path(path);
	
	int fd;
	int res;

	(void) fi;
	fd = open(full_path, O_RDONLY);
	if (fd == -1)
		return -errno;

	res = pread(fd, buf, size, offset);
	if (res == -1)
		res = -errno;

	close(fd);
	return res;
}

static int pa5_encfs_write(const char *path, const char *buf, size_t size,
		     off_t offset, struct fuse_file_info *fi)
{
	if (debug) {
		printf("Entering pa5_encfs_write\n");
	}
	
	int fd;
	int res;
	
	char* full_path = absolute_path(path);

	(void) fi;
	fd = open(full_path, O_WRONLY);
	if (fd == -1)
		return -errno;

	res = pwrite(fd, buf, size, offset);
	if (res == -1)
		res = -errno;

	close(fd);
	return res;
}

static int pa5_encfs_statfs(const char *path, struct statvfs *stbuf)
{
	if (debug) {
		printf("Entering pa5_encfs_statfs\n");
	}
	
	int res;
	
	char* full_path = absolute_path(path);

	res = statvfs(full_path, stbuf);
	if (res == -1)
		return -errno;

	return 0;
}

static int pa5_encfs_create(const char* path, mode_t mode, struct fuse_file_info* fi) {
	
	if (debug) {
		printf("Entering pa5_encfs_create\n");
	}
	
	(void) fi;
	
    int res;
    
    char* full_path = absolute_path(path);
    
    res = creat(full_path, mode);
    if(res == -1) {
		printf("Failed to create path");
		return -errno;
	}
	

    close(res);

    return 0;
}


static int pa5_encfs_release(const char *path, struct fuse_file_info *fi)
{
	/* Just a stub.	 This method is optional and can safely be left
	   unimplemented */
	   
	if (debug) {
		printf("Entering pa5_encfs_release\n");
	}

	(void) path;
	(void) fi;
	return 0;
}

static int pa5_encfs_fsync(const char *path, int isdatasync,
		     struct fuse_file_info *fi)
{
	/* Just a stub.	 This method is optional and can safely be left
	   unimplemented */
	if (debug) {
		printf("Entering pa5_encfs_fsync\n");
	}
	   

	(void) path;
	(void) isdatasync;
	(void) fi;
	return 0;
}

#ifdef HAVE_SETXATTR
static int pa5_encfs_setxattr(const char *path, const char *name, const char *value,
			size_t size, int flags)
{
	if (debug) {
		printf("Entering pa5_encfs_setxattr\n");
	}	
	
	char* full_path = absolute_path(path);
	
	int res = lsetxattr(full_path, name, value, size, flags);
	if (res == -1)
		return -errno;
	return 0;
}

static int pa5_encfs_getxattr(const char *path, const char *name, char *value,
			size_t size)
{
	if (debug) {
		printf("Entering pa5_encfs_getxattr\n");
	}
	
	char* full_path = absolute_path(path);
	
	int res = lgetxattr(full_path, name, value, size);
	if (res == -1)
		return -errno;
	return res;
}

static int pa5_encfs_listxattr(const char *path, char *list, size_t size)
{
	if (debug) {
		printf("Entering pa5_encfs_listxattr\n");
	}
	
	char* full_path = absolute_path(path);
	
	int res = llistxattr(full_path, list, size);
	if (res == -1)
		return -errno;
	return res;
}

static int pa5_encfs_removexattr(const char *path, const char *name)
{
	if (debug) {
		printf("Entering pa5_encfs_removexattr\n");
	}
	
	char* full_path = absolute_path(path);
	
	int res = lremovexattr(full_path, name);
	if (res == -1)
		return -errno;
	return 0;
}


#endif /* HAVE_SETXATTR */

static struct fuse_operations pa5_encfs_oper = {
	.getattr	= pa5_encfs_getattr,
	.access		= pa5_encfs_access,
	.readlink	= pa5_encfs_readlink,
	.readdir	= pa5_encfs_readdir,
	.mknod		= pa5_encfs_mknod,
	.mkdir		= pa5_encfs_mkdir,
	.symlink	= pa5_encfs_symlink,
	.unlink		= pa5_encfs_unlink,
	.rmdir		= pa5_encfs_rmdir,
	.rename		= pa5_encfs_rename,
	.link		= pa5_encfs_link,
	.chmod		= pa5_encfs_chmod,
	.chown		= pa5_encfs_chown,
	.truncate	= pa5_encfs_truncate,
	.utimens	= pa5_encfs_utimens,
	.open		= pa5_encfs_open,
	.read		= pa5_encfs_read,
	.write		= pa5_encfs_write,
	.statfs		= pa5_encfs_statfs,
	.create     = pa5_encfs_create,
	.release	= pa5_encfs_release,
	.fsync		= pa5_encfs_fsync,
#ifdef HAVE_SETXATTR
	.setxattr	= pa5_encfs_setxattr,
	.getxattr	= pa5_encfs_getxattr,
	.listxattr	= pa5_encfs_listxattr,
	.removexattr= pa5_encfs_removexattr,
#endif
};



int main(int argc, char *argv[])
{
	umask(0);
	if ((argc < 3) || (argv[argc-2][0] == '-') || (argv[argc-1][0] == '-')) {
		fprintf(stderr, "usage:  pa5-encfs [FUSE and mount options] rootDir mountPoint\n");
		exit(1);
	}
	
	root_dir = realpath(argv[argc - 1], NULL);
	
	if (root_dir == NULL){
		fprintf(stderr, "Enter a valid root directory name\n");
	}
	
	
	printf("path = %s\n", root_dir);
	argc--;
	return fuse_main(argc, argv, &pa5_encfs_oper, NULL);
}
