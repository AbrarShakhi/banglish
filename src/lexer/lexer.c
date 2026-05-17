#include "lexer.h"
#include "utils/memory.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

Lexer *lexer_create(const char *input) {
  Lexer *lx = mem_alloc(sizeof(Lexer));
  lx->input = input;
  lx->pos = 0;
  lx->len = strlen(input);
  lx->line = 1;
  lx->col = 1;
  lx->pushed = NULL;
  return lx;
}

void lexer_destroy(Lexer *lx) {
  if (!lx)
    return;
  token_destroy(lx->pushed);
  mem_free(lx);
}
