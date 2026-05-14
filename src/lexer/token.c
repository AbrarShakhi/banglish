#include "token.h"
#include "utils/memory.h"
#include <stdlib.h>

Token *token_new(TokenType type, const char *value, int line, int col) {
  Token *tok = mem_alloc(sizeof(Token));
  tok->type = type;
  tok->value = value ? mem_strdup(value) : NULL;
  tok->line = line;
  tok->col = col;
  return tok;
}

void token_destroy(Token *tok) {
  if (!tok)
    return;
  mem_free(tok->value);
  mem_free(tok);
}
