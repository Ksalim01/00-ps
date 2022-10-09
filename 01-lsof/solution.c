#include <ctype.h>
#include <dirent.h>
#include <errno.h>
#include <fcntl.h>
#include <fs_malloc.h>
#include <fs_string.h>
#include <limits.h>
#include <solution.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>

int is_num(const char* path) { return atoi(path); }

void replace_tail(char* path, const char* new_tail) {
  for (size_t i = strlen(path); i > 0; --i) {
    if (path[i] != '/') {
	  continue;
	}

	for (size_t j = 0; j <= strlen(new_tail); ++j) {
	  path[i + 1 + j] = new_tail[j];
	}
	return;
  }
}

void readlink_pid(const char* path) {
  struct stat sb;
  lstat(path, &sb);
  ssize_t bufsiz = sb.st_size + 1;

  char* fdlink = fs_xmalloc(bufsiz);
  fdlink[0] = '\0';

  if (readlink(path, fdlink, bufsiz) > 0) {
    report_file(fdlink);
  } else {
    report_error(path, errno);
  }

  fs_xfree(fdlink);
}

void lsof(void) {
  const char* proccess_path = "/proc/";
  DIR* proc_fd = opendir(proccess_path);
  if (proc_fd == NULL) {
    report_error(proccess_path, errno);
    return;
  }

  char path[100];
  struct dirent* dir;

  while ((dir = readdir(proc_fd)) != NULL) {
    if (is_num(dir->d_name) == 0) {
      continue;
    }

    path[0] = '\0';
    snprintf(path, sizeof(path), "/proc/%u/fd/", atoi(dir->d_name));

    DIR* pid_fd = opendir(path);
    if (pid_fd == NULL) {
      report_error(path, errno);
      continue;
    }

    while ((dir = readdir(pid_fd)) != NULL) {
      if (dir->d_name[0] == '.') {
        continue;
      }

      replace_tail(path, dir->d_name);
      readlink_pid(path);
    }

    closedir(pid_fd);
  }
  closedir(proc_fd);
}
