#pragma once
#include "core/shell.h"

int builtin_jobs(ShellState *sh, int argc, char **argv);
int builtin_fg(ShellState *sh, int argc, char **argv);
int builtin_bg(ShellState *sh, int argc, char **argv);
