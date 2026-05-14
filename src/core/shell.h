#pragma once
#include "env.h"

/* Opaque types — include their headers when you need to use them */
struct JobTable;   /* exec/job.h     */
struct AliasTable; /* builtin/alias.h */
struct FuncTable;  /* scripting/funcstore.h */

typedef struct ShellState {
  int interactive;
  int last_status; /* $? */
  EnvTable *env;
  struct JobTable *jobs;
  struct AliasTable *aliases; /* NULL until first alias cmd */
  struct FuncTable *funcs;    /* NULL until first def      */
  char *history_path;         /* ~/.banglish_history; heap-owned */
  int script_argc;
  char **script_argv;
  int secondary_prompt; /* show "> " instead of full prompt */
} ShellState;

ShellState *shell_init(int argc, char **argv);
void shell_destroy(ShellState *sh);
ShellState *shell_get_global(void); /* for use in signal handlers */
