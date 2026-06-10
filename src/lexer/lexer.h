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
Token *lexer_next(Lexer *lx); /* caller owns returned Token */
Token *lexer_peek(Lexer *lx); /* owned by lexer; valid until next call */
void lexer_push_back(Lexer *lx, Token *tok);

/* Collect heredoc content: skip to next line then read until a line that
   exactly matches 'delim'. Advances lx->pos past the closing delimiter line.
   Returns heap-allocated content string, or NULL if delimiter not found
   (input incomplete — caller should set parser->incomplete). */
char *lexer_slurp_heredoc(Lexer *lx, const char *delim);
