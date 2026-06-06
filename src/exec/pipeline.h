#pragma once
#include "core/shell.h"
#include "parser/ast.h"

int exec_pipeline(ShellState *sh, AstNode *pipe_node);
int exec_simple_cmd(ShellState *sh, const AstNode *cmd_node,
                    char **expanded_argv);
