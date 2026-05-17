#include "parser.h"
#include "utils/memory.h"

Parser *parser_create(Lexer *lx, struct ShellState *sh) {
  Parser *p = mem_alloc(sizeof(Parser));
  p->lx = lx;
  p->sh = sh;
  p->error = 0;
  p->incomplete = 0;
  return p;
}

void parser_destroy(Parser *p) { mem_free(p); }
