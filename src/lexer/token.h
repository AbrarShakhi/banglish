#pragma once

typedef enum {
  /* Literals */
  TOK_WORD,    /* any word/quoted-string/variable reference */
  TOK_NEWLINE, /* \n  — statement terminator in scripts */
  TOK_EOF,

  /* Pipeline / control */
  TOK_PIPE,     /* |   */
  TOK_PIPE_ERR, /* |&  — pipe stderr+stdout */
  TOK_AND,      /* &&  */
  TOK_OR,       /* ||  */
  TOK_SEMI,     /* ;   */
  TOK_AMP,      /* &   — background */

  /* Grouping */
  TOK_LPAREN, /* (   */
  TOK_RPAREN, /* )   */
  TOK_LBRACE, /* {   */
  TOK_RBRACE, /* }   */

  /* Redirection */
  TOK_REDIR_IN,          /* <   */
  TOK_REDIR_OUT,         /* >   */
  TOK_REDIR_APPEND,      /* >>  */
  TOK_REDIR_ERR,         /* 2>  */
  TOK_REDIR_ERR_APPEND,  /* 2>> */
  TOK_REDIR_BOTH,        /* &>  */
  TOK_REDIR_BOTH_APPEND, /* &>> */
  TOK_HEREDOC,           /* <<  */
} TokenType;

typedef struct {
  TokenType type;
  char *value; /* heap-owned; non-NULL only for TOK_WORD */
  int line;
  int col;
} Token;

Token *token_new(TokenType type, const char *value, int line, int col);
void token_destroy(Token *tok);
