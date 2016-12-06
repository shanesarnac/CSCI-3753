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
#define ENCRYPT 1
#define DECRYPT 0
#define PASS_THROUGH -1
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

#include "aes-crypt.h"

char debug = 0;
char * mirror_dir = NULL;
char * mount_point = NULL;
char * key_phrase = NULL;
const char * log_file_path = "/media/storage/Documents/College/Computer Programming/CSCI-3753/HW5/Test/logfile.txt";

// returns 1 if the file at file_location has a user.encfs attribute set to true
// returns 0 otherwise
char is_encrypted(const char* file_location) {
	char file_attributes[5];
	getxattr(file_location, "user.encfs", file_attributes, 5*sizeof(char));
	
	return (strcmp(file_attributes, "true") == 0);
}

// returns 1 if encryption attribute is successfully set, 0 otherwise
char set_encryption_attr(const char* file_location) {
	return !setxattr(file_location, "user.encfs", "true", 5*sizeof(char), 0);
}


char* absolute_path(const char* original_path) {
	if (debug) {
		printf("Entering absolute_path\n");
	}
	
	size_t len = strlen(original_path) + strlen(mirror_dir) + 1;
	char * root_path = malloc(len*sizeof(char));
	
	strcpy(root_path, mirror_dir);
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
	
	(void) fi;
	char* full_path = absolute_path(path);
	int res, encryption_action;
	
	FILE *read_file, *temp_file;
	read_file = fopen(full_path, "r");
	temp_file = tmpfile();
	
	
	if (is_encrypted(full_path) == 1) {
		encryption_action = DECRYPT;
	}
	else {
		encryption_action = PASS_THROUGH;
	}
	
	if(!do_crypt(read_file, temp_file, encryption_action, key_phrase)) {
		fprintf(stderr, "Unable to decrypt file\n");
		return errno;
	}
	
	fflush(temp_file);
	fseek(temp_file, offset, SEEK_SET);
	
	res = fread(buf, 1, size, temp_file);
	if (res < 0) {
		fprintf(stderr, "Unable to read file\n");
		res = errno;
	}
	
	fclose(temp_file);
	fclose(read_file);
	
	
	return res;
}

static int pa5_encfs_write(const char *path, const char *buf, size_t size,
		     off_t offset, struct fuse_file_info *fi)
{
	/*(void) fi;
	FILE *path_ptr, *tmpf;
	char *full_path;
	int res, action;
	int tmpf_descriptor;

	full_path = absolute_path(path);
	path_ptr = fopen(full_path, "r+");
	tmpf = tmpfile();
	tmpf_descriptor = fileno(tmpf);


	// Something went terribly wrong if this is the case. 
	if (path_ptr == NULL || tmpf == NULL)
		return -errno;
	
	fseek(path_ptr, 0, SEEK_END);
	int length = ftell(path_ptr);
	fseek(path_ptr, 0, SEEK_SET);

	// if the file to write to exists, read it into the tempfile 
	if (access(path, R_OK) == 0 && length > 0) {
		action = is_encrypted(full_path) ? DECRYPT : PASS_THROUGH;
		if (do_crypt(path_ptr, tmpf, action, key_phrase) == 0)
			return --errno;

		rewind(path_ptr);
		rewind(tmpf);
	}

	// Read our tmpfile into the buffer.
	res = pwrite(tmpf_descriptor, buf, size, offset);
	if (res == -1)
		res = -errno;

	// Either encrypt, or just move along. 
	action = is_encrypted(path) ? ENCRYPT : PASS_THROUGH;

	if (do_crypt(tmpf, path_ptr, action, key_phrase) == 0)
		return -errno;

	fclose(tmpf);
	fclose(path_ptr);

	return res; */
	
	if (debug) {
		printf("Entering pa5_encfs_write\n");
	}
	
	(void) fi;
	int res, encryption_action;
	char* full_path = absolute_path(path);
	
	FILE * write_file, *temp_file;
	write_file = fopen(full_path, "w+");
	temp_file = tmpfile();
	
	if (is_encrypted(full_path)) {
		encryption_action = DECRYPT;
	}
	else {
		encryption_action = PASS_THROUGH;
	}
	
	if(!do_crypt(write_file, temp_file, encryption_action, key_phrase)) {
		fprintf(stderr, "Could not decrypt file\n");
		return errno;
	}
	
	fseek(temp_file, offset, SEEK_SET);
	
	res = fwrite(buf, 1, size, temp_file);
	if (res < 0) {
		fprintf(stderr, "Unable to write to file\n");
		return errno;
	}
	
	fseek(temp_file, offset, SEEK_SET);
	fseek(write_file, offset, SEEK_SET);
	
	if(!do_crypt(temp_file, write_file, encryption_action, key_phrase)) {
		fprintf(stderr, "Could not write to file\n");
		return errno;
	}
	
	fclose(write_file);
	fclose(temp_file);
	
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
    FILE * file_in;
    
    char* full_path = absolute_path(path);
    
    res = creat(full_path, mode);
    
    if(res == -1) {
		printf("Failed to create path");
		return -errno;
	}
	
    close(res);
    
    file_in = fopen(full_path, "r+");   
    
    if (do_crypt(file_in, file_in, ENCRYPT , key_phrase)) {
		if (!set_encryption_attr(full_path)) {
			fprintf(stderr, "Failed to set encryption attribute\n");
		}
	}
	else {
		fprintf(stderr, "Failed to encrypt file\n");
	}
	
	fclose(file_in);

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


// ./pa5-encfs [options] <key phrase> <mirror directory> <mount point>
int main(int argc, char *argv[])
{
	umask(0);
	if ((argc < 4) || (argv[argc-2][0] == '-') || (argv[argc-1][0] == '-')) {
		fprintf(stderr, "usage:  pa5-encfs [FUSE and mount options] <Key Phrase> <Mirror Directory> <Mount Point>\n");
		exit(1);
	}
	
	mount_point = realpath(argv[argc - 1], NULL);
	mirror_dir = realpath(argv[argc - 2], NULL);
	key_phrase = argv[argc - 3];
	
	argv[argc -3] = argv[argc - 1];
	argv[argc - 1] = NULL;
	argc--;
	
	if (mirror_dir == NULL){
		fprintf(stderr, "Enter a valid root directory name\n");
	}
	if (mount_point == NULL) {
		fprintf(stderr, "Enter valid mount point\n");
	}
	if (key_phrase == NULL) {
		fprintf(stderr, "Enter a passphrase\n");
	}
	
	printf("key phrase = %s\n", key_phrase);
	printf("mirror directory = %s\n", mirror_dir);
	printf("mount point = %s\n", mount_point);
	
	argc--;
	return fuse_main(argc, argv, &pa5_encfs_oper, NULL);
}
