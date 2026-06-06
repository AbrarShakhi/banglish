#include "lexer.h"
#include "utils/memory.h"
#include "utils/vec.h"
#include <cstddef>
#include <ctype.h>
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

static char lx_advance(Lexer *lx) {
  char c = lx->input[lx->pos++];
  if (c == '\n') {
    lx->line++;
    lx->col = 1;
  } else {
    lx->col++;
  }
  return c;
}

/* Skip spaces and tabs. */
static void lx_skip_spaces(Lexer *lx) {
  while (lx->pos < lx->len) {
    char c = lx->input[lx->pos];
    if (c == ' ' || c == '\t')
      lx_advance(lx);
    else
      break;
  }
}

Token *lexer_next(Lexer *lx) {
  if (lx->pushed) {
    Token *t = lx->pushed;
    lx->pushed = NULL;
    return t;
  }

  lx_skip_spaces(lx);

  int save_line = lx->line;
  int save_col = lx->col;

  if (lx->pos >= lx->len)
    return token_new(TOK_EOF, NULL, save_line, save_col);

  char c = lx->input[lx->pos];

  /* Comment: skip to end of line */
  if (c == '#') {
    while (lx->pos < lx->len && lx->input[lx->pos] != '\n')
      lx_advance(lx);
    return lexer_next(lx); /* recurse to get next real token */
  }

  /* Newline */
  if (c == '\n') {
    lx_advance(lx);
    return token_new(TOK_NEWLINE, NULL, save_line, save_col);
  }

  /* Semicolon */
  if (c == ';') {
    lx_advance(lx);
    return token_new(TOK_SEMI, NULL, save_line, save_col);
  }

  /* Left / right brace/paren */
  if (c == '{') {
    lx_advance(lx);
    return token_new(TOK_LBRACE, NULL, save_line, save_col);
  }
  if (c == '}') {
    lx_advance(lx);
    return token_new(TOK_RBRACE, NULL, save_line, save_col);
  }
  if (c == '(') {
    lx_advance(lx);
    return token_new(TOK_LPAREN, NULL, save_line, save_col);
  }
  if (c == ')') {
    lx_advance(lx);
    return token_new(TOK_RPAREN, NULL, save_line, save_col);
  }

  /* Pipe: |, ||, |& */
  if (c == '|') {
    // TODO: PIPING MACHANISM
  }

  /* Ampersand: &, &&, &>, &>> */
  if (c == '&') {
    // TODO: Condition
  }

  /* Input redirect: <, << */
  if (c == '<') {
    lx_advance(lx);
    if (lx->input[lx->pos] == '<') {
      lx_advance(lx);
      return token_new(TOK_HEREDOC, NULL, save_line, save_col);
    }
    return token_new(TOK_REDIR_IN, NULL, save_line, save_col);
  }

  /* Output redirect: >, >> */
  if (c == '>') {
    lx_advance(lx);
    if (lx->input[lx->pos] == '>') {
      lx_advance(lx);
      return token_new(TOK_REDIR_APPEND, NULL, save_line, save_col);
    }
    return token_new(TOK_REDIR_OUT, NULL, save_line, save_col);
  }

  /* 2> and 2>>: digit followed immediately by > */
  if (isdigit((unsigned char)c) && lx->input[lx->pos] == '>') {
    /* For now only handle fd 2 specially; others fall through to word */
    if (c == '2') {
      lx_advance(lx); /* consume '2' */
      lx_advance(lx); /* consume '>' */
      if (lx->input[lx->pos] == '>') {
        lx_advance(lx);
        return token_new(TOK_REDIR_ERR_APPEND, NULL, save_line, save_col);
      }
      return token_new(TOK_REDIR_ERR, NULL, save_line, save_col);
    }
  }

  /* Everything else is a word */
  Token *w = NULL; // lexer word will create this
  /* An empty word means we hit something unexpected; return it anyway */
  return w;
}

Token *lexer_peek(Lexer *lx) {
  if (!lx->pushed)
    lx->pushed = lexer_next(lx);
  return lx->pushed;
}

void lexer_push_back(Lexer *lx, Token *tok) {
  token_destroy(lx->pushed);
  lx->pushed = tok;
}

char *lexer_slurp_heredoc(Lexer *lx, const char *delim) {
  size_t dlen = strlen(delim);

  /* Skip to start of next line (past the '\n' after << DELIM ...) */
  size_t scan = lx->pos;
  while (scan < lx->len && lx->input[scan] != '\n')
    scan++;
  if (scan >= lx->len)
    return NULL; /* no newline yet — incomplete */
  scan++;        /* consume '\n' */

  /* Collect content lines until we see a line that is exactly delim */
  Vec *content = vec_create();
  int found = 0;

  while (scan < lx->len) {
    size_t line_start = scan;
    while (scan < lx->len && lx->input[scan] != '\n')
      scan++;
    size_t line_len = scan - line_start;
    if (scan < lx->len)
      scan++; /* consume '\n' */

    if (line_len == dlen && strncmp(lx->input + line_start, delim, dlen) == 0) {
      lx->pos = scan; /* advance lexer past the heredoc body */
      found = 1;
      break;
    }

    for (size_t i = 0; i < line_len; i++) {
      char *ch = mem_alloc(1);
      *ch = lx->input[line_start + i];
      vec_push(content, ch);
    }
    char *nl = mem_alloc(1);
    *nl = '\n';
    vec_push(content, nl);
  }

  if (!found) {
    for (int j = 0; j < content->count; j++)
      mem_free(content->data[j]);
    vec_destroy(content);
    return NULL; /* delimiter not found — input is incomplete */
  }

  char *nul = mem_alloc(1);
  *nul = '\0';
  vec_push(content, nul);
  size_t total = (size_t)content->count;
  char *result = mem_alloc(total);
  for (int j = 0; j < content->count; j++) {
    result[j] = *(char *)content->data[j];
    mem_free(content->data[j]);
  }
  vec_destroy(content);
  return result;
}
