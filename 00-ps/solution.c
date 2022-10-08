#include <solution.h>
#include <ctype.h>
#include <dirent.h>
#include <errno.h>
#include <fcntl.h>
#include <fs_malloc.h>
#include <fs_string.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

const size_t MAX_ARGS = 1 << 17;

void clear(char** arr) {
  if (arr == NULL) {
    return;
  }
  for (char** i = arr; *i != NULL; ++i) {
    fs_xfree(*i);
  }
  fs_xfree(arr);
}

char* executable_file(const char* path) {
  char full_path[50];
  strcpy(full_path, path);
  strcat(full_path, "/exe");
  return realpath(full_path, NULL);
}

char** parse(const char* path) {
  int fd = open(path, O_RDONLY);
  if (fd < 0) {
    report_error(path, errno);
    return NULL;
  }

  char* buffer = fs_xmalloc(MAX_ARGS);
  ssize_t read_bytes = read(fd, buffer, MAX_ARGS);
  if (read_bytes < 0) {
    fs_xfree(buffer);
    report_error(path, errno);
    return NULL;
  }

  size_t args_size = 16;
  size_t j = 0;
  char** args = fs_xmalloc(args_size * sizeof(char*));
  for (ssize_t i = 0; i < read_bytes; i += strlen(buffer + i) + 1) {
    if (j == args_size - 1) {
      args_size *= 2;
      args = fs_xrealloc(args, args_size * sizeof(char*));
    }

    args[j] = fs_xmalloc(strlen(buffer + i) + 1);
    strcpy(args[j], buffer + i);
    ++j;
  }
  args[j] = NULL;

  fs_xfree(buffer);
  return args;
}

char** cmdline_args(const char* path) {
  char full_path[50];
  snprintf(full_path, sizeof(full_path), "%s/cmdline", path);

  return parse(full_path);
}

char** env_vars(const char* path) {
  char full_path[50];
  snprintf(full_path, sizeof(full_path), "%s/environ", path);

  return parse(full_path);
}

int is_proccess(const char* path) { return atoi(path); }

void ps(void) {
  struct dirent* dir;
  const char* proccess_path = "/proc/";
  DIR* proc = opendir(proccess_path);
  if (proc == NULL) {
    report_error(proccess_path, errno);
  }

  char path[50];

  while ((dir = readdir(proc)) != NULL) {
    path[0] = '\0';
    strcpy(path, proccess_path);

    if (is_proccess(dir->d_name) == 0) {
      continue;
    }

    strcat(path, dir->d_name);

    pid_t pid = atoi(dir->d_name);
    char* exe = executable_file(path);
    char** argv = cmdline_args(path);
    char** envp = env_vars(path);

    if (argv == NULL || envp == NULL) {
      fs_xfree(exe);
      clear(argv);
      clear(envp);
      closedir(proc);
      report_error("aboba", 1);
      abort();
    }
    report_process(pid, exe, argv, envp);

    fs_xfree(exe);
    clear(argv);
    clear(envp);
  }
  closedir(proc);
}