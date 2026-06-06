#pragma once
#include "parser/ast.h"

typedef struct {
  int saved[3]; /* dup'd copies of fd 0, 1, 2 */
} RedirSave;

int redirects_apply(const Redir *redirs, int n, RedirSave *save);
void redirects_restore(const RedirSave *save);
