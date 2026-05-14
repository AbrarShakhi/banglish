#include "registry.h"

#include <string.h>

#include "alias.h"
#include "cd.h"
#include "echo.h"
#include "exit_cmd.h"
#include "help.h"
#include "history_cmd.h"
#include "jobs_cmd.h"
#include "pwd.h"
#include "source.h"
#include "type_cmd.h"
#include "unset.h"
#include "which_cmd.h"

typedef struct {
  const char *name;
  BuiltinFn fn;
} BuiltinEntry;

static const BuiltinEntry g_builtins[] = {
    {"cd", builtin_cd},
    {"exit", builtin_exit},
    {"echo", builtin_echo},
    {"pwd", builtin_pwd},
    {"help", builtin_help},
    {"jobs", builtin_jobs},
    {"fg", builtin_fg},
    {"bg", builtin_bg},
    {"source", builtin_source},
    {".", builtin_source}, /* POSIX alias */
    {"unset", builtin_unset},
    {"type", builtin_type},
    {"which", builtin_which},
    {"alias", builtin_alias},
    {"unalias", builtin_unalias},
    {"history", builtin_history},
    {NULL, NULL},
};

void builtins_register_all(void) {
  /* Table is static; no dynamic registration needed. */
}

BuiltinFn builtin_find(const char *name) {
  for (int i = 0; g_builtins[i].name; i++)
    if (strcmp(g_builtins[i].name, name) == 0)
      return g_builtins[i].fn;
  return NULL;
}
