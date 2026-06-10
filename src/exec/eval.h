#pragma once
#include "core/shell.h"
#include "parser/ast.h"

int eval_node(ShellState *sh, AstNode *node);
int eval_string(ShellState *sh, const char *src);
int eval_file(ShellState *sh, const char *path);
