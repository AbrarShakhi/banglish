#pragma once
#include "core/shell.h"

void prompt_init(ShellState *sh);
void prompt_teardown(ShellState *sh);
char *prompt_readline(ShellState *sh);
/* caller frees with free() (readline API) */
