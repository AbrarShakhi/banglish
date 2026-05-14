#pragma once
#include "core/shell.h"

int eval_string(ShellState *sh, const char *src);
int eval_file(ShellState *sh, const char *path);
