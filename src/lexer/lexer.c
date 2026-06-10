#include "lexer.h"
#include "utils/memory.h"
#include "utils/vec.h"
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

static char lx_peek_char(const Lexer *lx) {
  return lx->pos < lx->len ? lx->input[lx->pos] : '\0';
}

static char lx_peek_char_at(const Lexer *lx, size_t offset) {
  return offset < lx->len ? lx->input[offset] : '\0';
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

/* Skip spaces and tabs (NOT newlines — they're meaningful). */
static void lx_skip_spaces(Lexer *lx) {
  while (lx->pos < lx->len) {
    char c = lx_peek_char(lx);
    if (c == ' ' || c == '\t' || c == '\r')
      lx_advance(lx);
    else
      break;
  }
}

/* Collect a word token, handling quotes, escapes, and $(…) / ${…}.
   Stops at whitespace or an unquoted operator character. */
static Token *lex_word(Lexer *lx) {
  int save_line = lx->line;
  int save_col = lx->col;
  Vec *buf = vec_create();

#define BUF_PUSH(c)                                                            \
  do {                                                                         \
    char *ch = mem_alloc(1);                                                   \
    *ch = (c);                                                                 \
    vec_push(buf, ch);                                                         \
  } while (0)

  while (lx->pos < lx->len) {
    char c = lx_peek_char(lx);

    if (c == '\\') {
      /* Escape: keep both \ and next char so expand can handle quoting/glob */
      lx_advance(lx);
      if (lx->pos < lx->len) {
        char escaped = lx_peek_char(lx);
        if (escaped == '\n') {
          /* Line continuation: skip newline, continue word */
          lx_advance(lx);
        } else {
          BUF_PUSH('\\');
          BUF_PUSH(lx_advance(lx));
        }
      }
      continue;
    }

    if (c == '\'') {
      /* Single-quote: everything literal until closing ' */
      lx_advance(lx); /* consume opening ' */
      BUF_PUSH('\'');
      while (lx->pos < lx->len) {
        char sc = lx_advance(lx);
        BUF_PUSH(sc);
        if (sc == '\'')
          break;
      }
      continue;
    }

    if (c == '"') {
      /* Double-quote: allow $ and \ expansion, but treat as single word */
      lx_advance(lx); /* consume opening " */
      BUF_PUSH('"');
      while (lx->pos < lx->len) {
        char dc = lx_peek_char(lx);
        if (dc == '"') {
          lx_advance(lx);
          BUF_PUSH('"');
          break;
        }
        if (dc == '\\') {
          lx_advance(lx);
          if (lx->pos < lx->len) {
            char ec = lx_advance(lx);
            BUF_PUSH('\\');
            BUF_PUSH(ec);
          }
          continue;
        }
        BUF_PUSH(lx_advance(lx));
      }
      continue;
    }

    if (c == '$') {
      /* Pass $VAR, ${…}, $(...) raw into the word; expand module handles it */
      BUF_PUSH(lx_advance(lx)); /* the $ */
      char nc = lx_peek_char(lx);
      if (nc == '{') {
        /* ${…}: collect until matching } */
        BUF_PUSH(lx_advance(lx));
        int depth = 1;
        while (lx->pos < lx->len && depth > 0) {
          char bc = lx_advance(lx);
          BUF_PUSH(bc);
          if (bc == '{')
            depth++;
          else if (bc == '}')
            depth--;
        }
      } else if (nc == '(') {
        /* $(…): collect until matching ) */
        BUF_PUSH(lx_advance(lx));
        int depth = 1;
        while (lx->pos < lx->len && depth > 0) {
          char pc = lx_peek_char(lx);
          if (pc == '\'') {
            /* single-quoted inside $(): pass raw */
            BUF_PUSH(lx_advance(lx));
            while (lx->pos < lx->len) {
              char sq = lx_advance(lx);
              BUF_PUSH(sq);
              if (sq == '\'')
                break;
            }
          } else if (pc == '"') {
            BUF_PUSH(lx_advance(lx));
            while (lx->pos < lx->len) {
              char dq = lx_advance(lx);
              BUF_PUSH(dq);
              if (dq == '"')
                break;
            }
          } else if (pc == '\\') {
            BUF_PUSH(lx_advance(lx));
            if (lx->pos < lx->len)
              BUF_PUSH(lx_advance(lx));
          } else {
            char rc = lx_advance(lx);
            BUF_PUSH(rc);
            if (rc == '(')
              depth++;
            else if (rc == ')')
              depth--;
          }
        }
      } else {
        /* $NAME or $? etc.: consume identifier chars */
        while (lx->pos < lx->len) {
          char vc = lx_peek_char(lx);
          if (isalnum((unsigned char)vc) || vc == '_' || vc == '?' ||
              vc == '#' || vc == '@' || vc == '*' || vc == '0')
            BUF_PUSH(lx_advance(lx));
          else
            break;
        }
      }
      continue;
    }

    /* Stop on unquoted operator chars or whitespace */
    if (c == ' ' || c == '\t' || c == '\r' || c == '\n')
      break;
    if (c == '|' || c == '&' || c == ';' || c == '<' || c == '>')
      break;
    if (c == '(' || c == ')' || c == '{' || c == '}')
      break;
    if (c == '#')
      break; /* comment */

    BUF_PUSH(lx_advance(lx));
  }

  /* Build the word string from buf */
  size_t word_len = (size_t)buf->count;
  char *word = mem_alloc(word_len + 1);
  for (int i = 0; i < buf->count; i++) {
    word[i] = *(char *)buf->data[i];
    mem_free(buf->data[i]);
  }
  word[word_len] = '\0';
  vec_destroy(buf);

#undef BUF_PUSH

  return token_new(TOK_WORD, word, save_line, save_col);
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

  char c = lx_peek_char(lx);

  /* Comment: skip to end of line */
  if (c == '#') {
    while (lx->pos < lx->len && lx_peek_char(lx) != '\n')
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
    lx_advance(lx);
    char nc = lx_peek_char(lx);
    if (nc == '|') {
      lx_advance(lx);
      return token_new(TOK_OR, NULL, save_line, save_col);
    }
    if (nc == '&') {
      lx_advance(lx);
      return token_new(TOK_PIPE_ERR, NULL, save_line, save_col);
    }
    return token_new(TOK_PIPE, NULL, save_line, save_col);
  }

  /* Ampersand: &, &&, &>, &>> */
  if (c == '&') {
    lx_advance(lx);
    char nc = lx_peek_char(lx);
    if (nc == '&') {
      lx_advance(lx);
      return token_new(TOK_AND, NULL, save_line, save_col);
    }
    if (nc == '>') {
      lx_advance(lx);
      if (lx_peek_char(lx) == '>') {
        lx_advance(lx);
        return token_new(TOK_REDIR_BOTH_APPEND, NULL, save_line, save_col);
      }
      return token_new(TOK_REDIR_BOTH, NULL, save_line, save_col);
    }
    return token_new(TOK_AMP, NULL, save_line, save_col);
  }

  /* Input redirect: <, << */
  if (c == '<') {
    lx_advance(lx);
    if (lx_peek_char(lx) == '<') {
      lx_advance(lx);
      return token_new(TOK_HEREDOC, NULL, save_line, save_col);
    }
    return token_new(TOK_REDIR_IN, NULL, save_line, save_col);
  }

  /* Output redirect: >, >> */
  if (c == '>') {
    lx_advance(lx);
    if (lx_peek_char(lx) == '>') {
      lx_advance(lx);
      return token_new(TOK_REDIR_APPEND, NULL, save_line, save_col);
    }
    return token_new(TOK_REDIR_OUT, NULL, save_line, save_col);
  }

  /* 2> and 2>>: digit followed immediately by > */
  if (isdigit((unsigned char)c) && lx_peek_char_at(lx, lx->pos + 1) == '>') {
    /* For now only handle fd 2 specially; others fall through to word */
    if (c == '2') {
      lx_advance(lx); /* consume '2' */
      lx_advance(lx); /* consume '>' */
      if (lx_peek_char(lx) == '>') {
        lx_advance(lx);
        return token_new(TOK_REDIR_ERR_APPEND, NULL, save_line, save_col);
      }
      return token_new(TOK_REDIR_ERR, NULL, save_line, save_col);
    }
  }

  /* Everything else is a word */
  Token *w = lex_word(lx);
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
