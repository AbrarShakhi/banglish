#include "shell.h"

#include <stdlib.h>
#include <unistd.h>

#include "env.h"
#include "exec/job.h"
#include "utils/memory.h"
#include "utils/str.h"

static ShellState *g_shell = NULL;

ShellState *shell_init(int argc, char **argv) {
  ShellState *sh = mem_alloc(sizeof(ShellState));
  sh->interactive = isatty(STDIN_FILENO);
  sh->last_status = 0;
  sh->env = env_create();
  sh->jobs = jobtable_create();
  sh->aliases = NULL;
  sh->funcs = NULL;
  sh->script_argc = argc;
  sh->script_argv = argv;
  sh->secondary_prompt = 0;

  const char *home = getenv("HOME");
  if (home && *home)
    sh->history_path = str_concat3(home, "/", ".banglish_history");
  else
    sh->history_path = mem_strdup(".banglish_history");

  g_shell = sh;
  return sh;
}

void shell_destroy(ShellState *sh) {
  if (!sh)
    return;
  env_destroy(sh->env);
  jobtable_destroy(sh->jobs);
  mem_free(sh->history_path);
  mem_free(sh);
  if (g_shell == sh)
    g_shell = NULL;
}

ShellState *shell_get_global(void) { return g_shell; }
