#include <errno.h>
#include <fuse.h>
#include <solution.h>
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>

const char *fs_path = "/hello";
const size_t max_pid_len = 10;

static int hellofs_readdir(const char *path, void *data, fuse_fill_dir_t filler,
                           off_t off, struct fuse_file_info *ffi) {
  (void)off;
  (void)ffi;  // because of errors unused-param
  if (strcmp(path, "/") != 0) return -ENOENT;

  filler(data, ".", NULL, 0);
  filler(data, "..", NULL, 0);
  filler(data, "hello", NULL, 0);
  return 0;
}

static int hellofs_read(const char *path, char *buf, size_t size, off_t off,
                        struct fuse_file_info *ffi) {
  (void)ffi;

  if (strcmp(path, fs_path) != 0) return -ENOENT;

  const char *format = "hello, %d\n";
  char file_contents[strlen(format) + max_pid_len];
  sprintf(file_contents, format, fuse_get_context()->pid);

  size_t len = strlen(file_contents);

  if (off >= (off_t)len) return 0;

  size = (off + size > len ? (size_t)(len - off) : size);
  memcpy(buf, file_contents + off, size);

  return size;
}

static int hellofs_open(const char *path, struct fuse_file_info *ffi) {
  if (strcmp(path, fs_path) != 0) return -ENOENT;

  if ((ffi->flags & O_ACCMODE) != O_RDONLY) return -EACCES;

  return 0;
}

static int hellofs_getattr(const char *path, struct stat *st) {
  if (strcmp(path, "/") == 0) {
    st->st_mode = 0755;
    st->st_nlink = 2;
  } else if (strcmp(path, fs_path) == 0) {
    st->st_mode = 0644;
    st->st_nlink = 1;
    st->st_size = 512;
  }

  return -ENOENT;
}

static int hellofs_write(const char *path, const char *buf, size_t size,
                         off_t off, struct fuse_file_info *ffi) {
  (void)path;
  (void)buf;
  (void)size;
  (void)off;
  (void)ffi;
  return -EROFS;
}

struct fuse_operations hellofs_ops = {.readdir = hellofs_readdir,
                                      .read = hellofs_read,
                                      .open = hellofs_open,
                                      .getattr = hellofs_getattr,
                                      .write = hellofs_write};

int helloworld(const char *mntp) {
  char *argv[] = {"exercise", "-f", (char *)mntp, NULL};
  return fuse_main(3, argv, &hellofs_ops, NULL);
}
