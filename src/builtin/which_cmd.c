#include "which_cmd.h"

#include <stdio.h>
#include <unistd.h>

#include "core/env.h"
#include "utils/memory.h"
#include "utils/str.h"

int builtin_which(ShellState *sh, int argc, char **argv) {
  if (argc < 2) {
    fprintf(stderr, "which: usage: which COMMAND...\n");
    return 1;
  }
  int ret = 0;
  for (int i = 1; i < argc; i++) {
    const char *name = argv[i];
    const char *path_env = env_get(sh->env, "PATH");
    if (!path_env)
      path_env = "/usr/bin:/bin";

    int n;
    char **dirs = str_split_colon(path_env, &n);
    int found = 0;
    for (int d = 0; d < n && !found; d++) {
      char *full = str_concat3(dirs[d], "/", name);
      if (access(full, X_OK) == 0) {
        puts(full);
        found = 1;
      }
      mem_free(full);
    }
    str_array_free(dirs);
    if (!found)
      ret = 1;
  }
  return ret;
}
