#include "type_cmd.h"

#include <stdio.h>
#include <unistd.h>

#include "core/env.h"
#include "registry.h"
#include "scripting/funcstore.h"
#include "utils/memory.h"
#include "utils/str.h"

int builtin_type(ShellState *sh, int argc, char **argv) {
  if (argc < 2) {
    fprintf(stderr, "type: usage: type NAME...\n");
    return 1;
  }
  int ret = 0;
  for (int i = 1; i < argc; i++) {
    const char *name = argv[i];

    if (builtin_find(name)) {
      printf("%s is a shell builtin\n", name);
      continue;
    }
    if (sh->funcs && functable_find((FuncTable *)sh->funcs, name)) {
      printf("%s is a shell function\n", name);
      continue;
    }
    /* Search PATH */
    const char *path_env = env_get(sh->env, "PATH");
    if (!path_env)
      path_env = "/usr/bin:/bin";
    int n;
    char **dirs = str_split_colon(path_env, &n);
    int found = 0;
    for (int d = 0; d < n && !found; d++) {
      char *full = str_concat3(dirs[d], "/", name);
      if (access(full, X_OK) == 0) {
        printf("%s is %s\n", name, full);
        found = 1;
      }
      mem_free(full);
    }
    str_array_free(dirs);
    if (!found) {
      fprintf(stderr, "type: %s: not found\n", name);
      ret = 1;
    }
  }
  return ret;
}
