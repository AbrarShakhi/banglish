#pragma once
#include "core/shell.h"

typedef int (*BuiltinFn)(ShellState *sh, int argc, char **argv);

void builtins_register_all(void); /* call once from shell_init context */
BuiltinFn builtin_find(const char *name);
