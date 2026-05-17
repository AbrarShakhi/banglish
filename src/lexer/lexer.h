#pragma once
#include "token.h"
#include <stddef.h>

typedef struct {
  const char *input; /* not owned; caller must keep alive */
  size_t pos;
  size_t len;
  int line;
  int col;
  Token *pushed; /* one-token pushback buffer */
} Lexer;

Lexer *lexer_create(const char *input);
void lexer_destroy(Lexer *lx);
