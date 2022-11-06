#include <ext2fs/ext2fs.h>
#include <solution.h>
#include <sys/types.h>
#include <sys/uio.h>
#include <unistd.h>

#define FIND_MODE 1
#define WRITE_MODE 2

const off_t INITIAL_OFFSET = 1024;
const int DIRECT_BLOCKS_NUM = 12;
size_t unread_bytes = 0;
size_t block_size = 0;

struct common {
  int traverse_mode;
  int inode_nr;
  char name[256];
  int file_type;
  int out;
};

struct iovec* create_iovec(size_t len) {
  struct iovec* iov = malloc(sizeof(struct iovec));
  iov->iov_base = malloc(len);
  iov->iov_len = len;

  return iov;
}

void clear_iovec(struct iovec* iov) {
  if (iov == NULL) return;
  free(iov->iov_base);
  free(iov);
}

int read_superblock(int img, struct iovec* sb) {
  return preadv(img, sb, 1, INITIAL_OFFSET);
}

int read_group_descr(int img, int inode_nr, struct ext2_super_block* sb,
                     struct iovec* descr) {
  block_size = 1024 << sb->s_log_block_size;

  size_t block_pos = descr->iov_len * (inode_nr / sb->s_inodes_per_group);

  off_t offset = block_size + block_pos + sb->s_first_data_block * block_size;

  return preadv(img, descr, 1, offset);
}

int read_inode(int img, int inode_nr, struct ext2_super_block* sb,
               struct ext2_group_desc* descr, struct iovec* inode) {
  size_t inode_pos = inode_nr % sb->s_inodes_per_group;

  off_t offset =
      descr->bg_inode_table * block_size + sb->s_inode_size * inode_pos;

  return preadv(img, inode, 1, offset);
}

void report_entry(struct iovec* buf, struct common* file) {
  char name[256];
  struct ext2_dir_entry_2* dir_entry;
  for (size_t i = 0; i < buf->iov_len; i += dir_entry->rec_len) {
    dir_entry = (struct ext2_dir_entry_2*)(buf->iov_base + i);
    memcpy(name, dir_entry->name, dir_entry->name_len);
    name[dir_entry->name_len] = '\0';

    if (strcmp(name, file->name) == 0) {
      file->inode_nr = dir_entry->inode - 1;
      file->file_type = dir_entry->file_type;
    }
  }
}

int read_block(int img, off_t offset, struct iovec* buf, struct common* file) {
  size_t should_read = (unread_bytes < block_size) ? unread_bytes : block_size;
  buf->iov_len = should_read;
  int read_bytes = preadv(img, buf, 1, offset);
  if (read_bytes < 0) return errno;

  unread_bytes -= read_bytes;

  if (file->traverse_mode == FIND_MODE)
    report_entry(buf, file);
  else if (file->traverse_mode == WRITE_MODE && writev(file->out, buf, 1) < 0)
    return errno;
  return 0;
}

int read_direct_blocks(int img, struct ext2_inode* inode, struct common* file) {
  struct iovec* buf = create_iovec(block_size);
  for (int i = 0; i < DIRECT_BLOCKS_NUM && inode->i_block[i] != 0; ++i) {
    if (read_block(img, inode->i_block[i] * block_size, buf, file) < 0) {
      clear_iovec(buf);
      return errno;
    }

    if (unread_bytes == 0) {
      break;
    }
  }
  clear_iovec(buf);
  return 0;
}

int read_indirect_blocks(int img, uint32_t ind_pos, struct common* file) {
  struct iovec* indirect = create_iovec(block_size);
  if (preadv(img, indirect, 1, ind_pos) < 0) {
    clear_iovec(indirect);
    return errno;
  }
  uint32_t* blocks_pos = indirect->iov_base;

  struct iovec* buf = create_iovec(block_size);
  for (size_t i = 0; i < block_size / 4 && blocks_pos[i] != 0; ++i) {
    if (read_block(img, blocks_pos[i] * block_size, buf, file) < 0) {
      clear_iovec(buf);
      return errno;
    }

    if (unread_bytes == 0) {
      break;
    }
  }

  clear_iovec(buf);
  clear_iovec(indirect);
  return 0;
}

int read_double_indirect_blocks(int img, uint32_t dint_pos,
                                struct common* file) {
  struct iovec* double_indirect = create_iovec(block_size);
  if (preadv(img, double_indirect, 1, dint_pos) < 0) {
    clear_iovec(double_indirect);
    return errno;
  }

  uint32_t* blocks_pos = double_indirect->iov_base;

  for (size_t i = 0; i < block_size / 4 && blocks_pos[i] != 0; ++i) {
    if (read_indirect_blocks(img, blocks_pos[i] * block_size, file) < 0) {
      return errno;
    }

    if (unread_bytes == 0) {
      break;
    }
  }
  clear_iovec(double_indirect);
  return 0;
}

int traverse_inode(int img, struct ext2_inode* inode, struct common* file) {
  unread_bytes = inode->i_size;

  if (read_direct_blocks(img, inode, file) < 0) return errno;
  if (read_indirect_blocks(img, inode->i_block[DIRECT_BLOCKS_NUM] * block_size,
                           file) < 0)
    return errno;
  if (read_double_indirect_blocks(
          img, inode->i_block[DIRECT_BLOCKS_NUM + 1] * block_size, file) < 0)
    return errno;

  return 0;
}

int traverse(struct iovec* super_block, struct iovec* group_descriptor,
             struct iovec* inode, struct common* file, int img) {
  int before_inode = file->inode_nr;
  file->inode_nr = -1;
  if (read_group_descr(img, before_inode, super_block->iov_base,
                       group_descriptor) < 0 ||
      read_inode(img, before_inode, super_block->iov_base,
                 group_descriptor->iov_base, inode) < 0 ||
      traverse_inode(img, inode->iov_base, file) < 0)
    return errno;

  if (file->inode_nr == -1 && file->traverse_mode == FIND_MODE) {
    errno = -ENOENT;
    return errno;
  }
  return 0;
}

size_t get_root(char* dest, const char* path) {
  size_t i = 0;
  for (; path[i] != '\0' && path[i] != '/'; ++i) {
    dest[i] = path[i];
  }
  dest[i] = '\0';
  // if (path[i] == '\0') return 0;
  return i;
}

void clear(struct iovec* super_block, struct iovec* group_descriptor,
           struct iovec* inode, struct common* file) {
  free(file);
  clear_iovec(inode);
  clear_iovec(super_block);
  clear_iovec(group_descriptor);
}

int dump_file(int img, const char* path, int out) {
  struct iovec* super_block = create_iovec(sizeof(struct ext2_super_block));
  if (read_superblock(img, super_block) < 0) {
    clear_iovec(super_block);
    return errno;
  }

  struct iovec* group_descriptor = create_iovec(sizeof(struct ext2_group_desc));
  struct iovec* inode = create_iovec(sizeof(struct ext2_inode));

  struct common* file = malloc(sizeof(struct common));
  file->out = -1;
  file->traverse_mode = FIND_MODE;
  file->file_type = EXT2_FT_DIR;
  file->inode_nr = 1;

  if (path[0] == '/') ++path;
  char file_name[256];
  size_t name_len = get_root(file_name, path);
  while (name_len > 0) {
    int is_dir = (strchr(path, '/') != NULL);
    strcpy(file->name, file_name);
    path += name_len;
    if (path[0] == '/') ++path;

    if (traverse(super_block, group_descriptor, inode, file, img) < 0) {
      clear(super_block, group_descriptor, inode, file);
      return errno;
    }
    
    if (is_dir && file->file_type != EXT2_FT_DIR) {
      clear(super_block, group_descriptor, inode, file);
      return -ENOTDIR;
    }
    name_len = get_root(file_name, path);
  }

  strcpy(file->name, file_name);
  file->out = out;
  file->traverse_mode = WRITE_MODE;

  if (traverse(super_block, group_descriptor, inode, file, img) < 0) {
    clear(super_block, group_descriptor, inode, file);
    return errno;
  }

  clear(super_block, group_descriptor, inode, file);
  return 0;
}