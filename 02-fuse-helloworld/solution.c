#include <errno.h>
#include <fuse.h>
#include <solution.h>
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>

const char *fs_path = "/hello";
const size_t max_pid_len = 10;

static int hellofs_readdir(const char *path, void *data, fuse_fill_dir_t filler,
                           off_t off, struct fuse_file_info *ffi,
                           enum fuse_readdir_flags flags) {
  (void)off;
  (void)ffi;
  (void)flags;  // because of errors unused-param
  if (strcmp(path, "/") != 0) return -ENOENT;

  filler(data, ".", NULL, 0, 0);
  filler(data, "..", NULL, 0, 0);
  filler(data, "hello", NULL, 0, 0);
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

static int hellofs_getattr(const char *path, struct stat *st,
                           struct fuse_file_info *ffi) {
  (void)ffi;
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

static int hellofs_write_buf(const char *path, struct fuse_bufvec *buf,
                             off_t off, struct fuse_file_info *ffi) {
  (void)path;
  (void)buf;
  (void)off;
  (void)ffi;
  return -EROFS;
}

static void *hellofs_init(struct fuse_conn_info *fci, struct fuse_config *fc) {
  (void)fci;
  (void)fc;
  return NULL;
}

static int hellofs_create(const char *path, mode_t mode,
                          struct fuse_file_info *ffi) {
  (void)path;
  (void)mode;
  (void)ffi;
  return -EROFS;
}

static int hellofs_mknod(const char *path, mode_t mode, dev_t fl) {
  (void)path;
  (void)mode;
  (void)fl;
  return -EROFS;
}

static int hellofs_mkdir(const char *path, mode_t mode) {
  (void)path;
  (void)mode;
  return -EROFS;
}

static const struct fuse_operations hellofs_ops = {
    .init = hellofs_init,
    .create = hellofs_create,
    .readdir = hellofs_readdir,
    .getattr = hellofs_getattr,
    .open = hellofs_open,
    .read = hellofs_read,
    .write = hellofs_write,
    .write_buf = hellofs_write_buf,
    .mknod = hellofs_mknod,
    .mkdir = hellofs_mkdir};

int helloworld(const char *mntp) {
  char *argv[] = {"exercise", "-f", (char *)mntp, NULL};
  return fuse_main(3, argv, &hellofs_ops, NULL);
}
