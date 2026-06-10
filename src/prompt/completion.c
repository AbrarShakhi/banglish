#include "completion.h"

#ifdef HAVE_READLINE

#include "builtin/registry.h"
#include "core/env.h"
#include "utils/memory.h"
#include "utils/str.h"
#include <dirent.h>
#include <readline/history.h>
#include <readline/readline.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

static ShellState *g_sh_for_completion = NULL;

/* ------------------------------------------------------------------ */
/* Command completion: built-ins + executables in $PATH               */
/* ------------------------------------------------------------------ */

static char *command_generator(const char *text, int state) {
  static int list_idx = 0;
  static char *path_env = NULL;
  static char **dirs = NULL;
  static int ndirs = 0;
  static int dir_idx = 0;
  static DIR *dirp = NULL;

  static const char *const builtins[] = {
      "cd",    "echo",    "exit",    "pwd",   "help", "jobs",
      "fg",    "bg",      "source",  "unset", "type", "which",
      "alias", "unalias", "history", ".",     NULL};

  if (!state) {
    list_idx = 0;
    dir_idx = 0;
    if (dirp) {
      closedir(dirp);
      dirp = NULL;
    }

    mem_free(path_env);
    const char *pe =
        g_sh_for_completion ? env_get(g_sh_for_completion->env, "PATH") : NULL;
    if (!pe)
      pe = "/usr/bin:/bin";
    path_env = mem_strdup(pe);

    str_array_free(dirs);
    dirs = str_split_colon(path_env, &ndirs);
  }

  size_t tlen = strlen(text);

  /* Phase 1: builtins */
  while (builtins[list_idx]) {
    const char *b = builtins[list_idx++];
    if (strncmp(b, text, tlen) == 0)
      return mem_strdup(b);
  }

  /* Phase 2: scan PATH directories */
  while (dir_idx < ndirs) {
    if (!dirp) {
      dirp = opendir(dirs[dir_idx]);
      if (!dirp) {
        dir_idx++;
        continue;
      }
    }
    struct dirent *de;
    while ((de = readdir(dirp))) {
      if (strncmp(de->d_name, text, tlen) != 0)
        continue;
      char *full = str_concat3(dirs[dir_idx], "/", de->d_name);
      int ok = (access(full, X_OK) == 0);
      mem_free(full);
      if (ok)
        return mem_strdup(de->d_name);
    }
    closedir(dirp);
    dirp = NULL;
    dir_idx++;
  }

  return NULL;
}

static char **banglish_completion(const char *text, int start, int end) {
  (void)end;
  rl_attempted_completion_over = 1;

  if (start == 0)
    return rl_completion_matches(text, command_generator);

  rl_attempted_completion_over = 0;
  return NULL;
}

void completion_init(ShellState *sh) {
  g_sh_for_completion = sh;
  rl_attempted_completion_function = banglish_completion;
}

#else

void completion_init(ShellState *sh) { (void)sh; }

#endif /* HAVE_READLINE */
