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

const char *token_type_name(TokenType type) {
  switch (type) {
  case TOK_WORD:
    return "WORD";
  case TOK_NEWLINE:
    return "NEWLINE";
  case TOK_EOF:
    return "EOF";
  case TOK_PIPE:
    return "|";
  case TOK_PIPE_ERR:
    return "|&";
  case TOK_AND:
    return "&&";
  case TOK_OR:
    return "||";
  case TOK_SEMI:
    return ";";
  case TOK_AMP:
    return "&";
  case TOK_LPAREN:
    return "(";
  case TOK_RPAREN:
    return ")";
  case TOK_LBRACE:
    return "{";
  case TOK_RBRACE:
    return "}";
  case TOK_REDIR_IN:
    return "<";
  case TOK_REDIR_OUT:
    return ">";
  case TOK_REDIR_APPEND:
    return ">>";
  case TOK_REDIR_ERR:
    return "2>";
  case TOK_REDIR_ERR_APPEND:
    return "2>>";
  case TOK_REDIR_BOTH:
    return "&>";
  case TOK_REDIR_BOTH_APPEND:
    return "&>>";
  case TOK_HEREDOC:
    return "<<";
  default:
    return "?";
  }
}
