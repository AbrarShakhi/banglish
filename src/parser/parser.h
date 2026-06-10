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

/* Parse one logical line (interactive). Returns PARSE_OK with *out_node set,
   PARSE_EOF if nothing to parse, PARSE_INCOMPLETE, or PARSE_ERROR. */
ParseResult parse_line(Parser *p, AstNode **out_node);

/* Parse an entire script (all statements until EOF). */
AstNode *parse_script(Parser *p);
