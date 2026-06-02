#include "cd.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "core/env.h"
#include "utils/memory.h"

int builtin_cd(ShellState *sh, int argc, char **argv) {
  char *target = NULL; /* heap-owned copy of destination path */

  if (argc < 2) {
    const char *home = env_get(sh->env, "HOME");
    if (!home) {
      fprintf(stderr, "cd: HOME not set\n");
      return 1;
    }
    target = mem_strdup(home);
  } else if (argc == 2 && strcmp(argv[1], "-") == 0) {
    const char *oldpwd = env_get(sh->env, "OLDPWD");
    if (!oldpwd) {
      fprintf(stderr, "cd: OLDPWD not set\n");
      return 1;
    }
    target = mem_strdup(oldpwd);
    printf("%s\n", target);
  } else {
    target = mem_strdup(argv[1]);
  }

  /* Save current dir as OLDPWD before chdir */
  char *cwd = getcwd(NULL, 0);
  if (cwd) {
    env_set(sh->env, "OLDPWD", cwd);
    free(cwd);
  }

  if (chdir(target) != 0) {
    perror(target);
    mem_free(target);
    return 1;
  }
  mem_free(target);

  /* Update PWD */
  char *new_cwd = getcwd(NULL, 0);
  if (new_cwd) {
    env_set(sh->env, "PWD", new_cwd);
    setenv("PWD", new_cwd, 1);
    free(new_cwd);
  }
  return 0;
}
