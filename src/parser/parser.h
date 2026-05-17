#pragma once
#include "ast.h"
#include "lexer/lexer.h"

/* Forward-declare ShellState to avoid pulling in the full header here */
struct ShellState;

typedef enum {
  PARSE_OK,
  PARSE_EOF,
  PARSE_INCOMPLETE, /* unclosed {, (, etc. — need more input */
  PARSE_ERROR,
} ParseResult;

typedef struct {
  Lexer *lx;             /* not owned */
  struct ShellState *sh; /* not owned; for alias lookup */
  int error;             /* set on syntax error */
  int incomplete;        /* set on unclosed constructs */
} Parser;

Parser *parser_create(Lexer *lx, struct ShellState *sh);
void parser_destroy(Parser *p);
