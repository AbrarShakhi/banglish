#include "parser.h"
#include "utils/memory.h"
#include "utils/str.h"
#include "utils/vec.h"
#include <stdio.h>
#include <string.h>

/* ------------------------------------------------------------------ */
/* Helpers                                                             */
/* ------------------------------------------------------------------ */

static Token *peek(Parser *p) { return lexer_peek(p->lx); }

static Token *advance(Parser *p) { return lexer_next(p->lx); }

static int at(Parser *p, TokenType t) { return peek(p)->type == t; }

/* Consume a token of type t; return 1 on success, 0 on failure. */
static int expect(Parser *p, TokenType t) {
  Token *tok = peek(p);
  if (tok->type != t) {
    fprintf(stderr, "bang-lish: line %d: expected '%s', got '%s'%s%s\n",
            tok->line, token_type_name(t), token_type_name(tok->type),
            tok->value ? " (" : "", tok->value ? tok->value : "");
    p->error = 1;
    return 0;
  }
  token_destroy(advance(p));
  return 1;
}

/* Skip any newlines. */
static void skip_newlines(Parser *p) {
  while (at(p, TOK_NEWLINE))
    token_destroy(advance(p));
}

/* Return 1 if the current word value matches keyword. */
static int is_keyword(Parser *p, const char *kw) {
  Token *t = peek(p);
  return t->type == TOK_WORD && t->value && strcmp(t->value, kw) == 0;
}

/* ------------------------------------------------------------------ */
/* Redirect parsing                                                    */
/* ------------------------------------------------------------------ */

static int is_redirect_token(TokenType t) {
  switch (t) {
  case TOK_REDIR_IN:
  case TOK_REDIR_OUT:
  case TOK_REDIR_APPEND:
  case TOK_REDIR_ERR:
  case TOK_REDIR_ERR_APPEND:
  case TOK_REDIR_BOTH:
  case TOK_REDIR_BOTH_APPEND:
  case TOK_HEREDOC:
    return 1;
  default:
    return 0;
  }
}

static Redir parse_one_redirect(Parser *p) {
  Redir r = {0};
  Token *rtok = advance(p); /* the redirect operator */

  switch (rtok->type) {
  case TOK_REDIR_IN:
    r.type = REDIR_IN;
    r.fd = 0;
    break;
  case TOK_REDIR_OUT:
    r.type = REDIR_OUT;
    r.fd = 1;
    break;
  case TOK_REDIR_APPEND:
    r.type = REDIR_APPEND;
    r.fd = 1;
    break;
  case TOK_REDIR_ERR:
    r.type = REDIR_ERR;
    r.fd = 2;
    break;
  case TOK_REDIR_ERR_APPEND:
    r.type = REDIR_ERR_APPEND;
    r.fd = 2;
    break;
  case TOK_REDIR_BOTH:
    r.type = REDIR_BOTH;
    r.fd = -1;
    break;
  case TOK_REDIR_BOTH_APPEND:
    r.type = REDIR_BOTH_APPEND;
    r.fd = -1;
    break;
  case TOK_HEREDOC:
    r.type = REDIR_HEREDOC;
    r.fd = 0;
    break;
  default:
    break;
  }
  int is_heredoc = (r.type == REDIR_HEREDOC);
  token_destroy(rtok);

  /* Fd-dup syntax: >&N or 2>&1 — the lexer splits '&N' into TOK_AMP + TOK_WORD
   */
  if (!is_heredoc && peek(p)->type == TOK_AMP) {
    Token *amp = advance(p);
    token_destroy(amp);
    if (peek(p)->type == TOK_WORD && peek(p)->value &&
        peek(p)->value[0] >= '0' && peek(p)->value[0] <= '9') {
      Token *fd_tok = advance(p);
      char buf[8];
      snprintf(buf, sizeof(buf), "&%s", fd_tok->value);
      r.target = mem_strdup(buf);
      token_destroy(fd_tok);
      return r;
    }
    /* Not fd-dup — put AMP back */
    lexer_push_back(p->lx, token_new(TOK_AMP, NULL, 0, 0));
  }

  /* Get the target filename / heredoc delimiter */
  if (!at(p, TOK_WORD)) {
    fprintf(stderr, "bang-lish: redirect: expected filename\n");
    p->error = 1;
    r.target = mem_strdup("");
    return r;
  }
  Token *ftok = advance(p);

  if (is_heredoc) {
    /* Read heredoc content from subsequent lines until delimiter */
    char *content = lexer_slurp_heredoc(p->lx, ftok->value);
    if (!content) {
      p->incomplete = 1;
      r.target = mem_strdup(ftok->value); /* placeholder for re-parse */
    } else {
      r.target = content;
      /* lexer_slurp_heredoc consumed the '\n' that ends this command line;
         push it back so parse_simple_cmd terminates the current command. */
      lexer_push_back(p->lx, token_new(TOK_NEWLINE, NULL, 0, 0));
    }
  } else {
    r.target = mem_strdup(ftok->value);
  }
  token_destroy(ftok);
  return r;
}

/* ------------------------------------------------------------------ */
/* Simple command                                                       */
/* ------------------------------------------------------------------ */

static AstNode *parse_simple_cmd(Parser *p) {
  Vec *words = vec_create();
  Vec *redirs = vec_create();

  while (1) {
    Token *tok = peek(p);

    if (is_redirect_token(tok->type)) {
      Redir *r = mem_alloc(sizeof(Redir));
      *r = parse_one_redirect(p);
      vec_push(redirs, r);
      continue;
    }

    if (tok->type != TOK_WORD)
      break;

    Token *wt = advance(p);
    vec_push(words, mem_strdup(wt->value));
    token_destroy(wt);
  }

  /* Collect trailing redirects */
  while (is_redirect_token(peek(p)->type)) {
    Redir *r = mem_alloc(sizeof(Redir));
    *r = parse_one_redirect(p);
    vec_push(redirs, r);
  }

  if (words->count == 0 && redirs->count == 0) {
    vec_destroy(words);
    vec_destroy(redirs);
    return NULL;
  }

  AstNode *n = ast_node_new(NODE_CMD);
  n->cmd.argc = words->count;
  n->cmd.argv = (char **)vec_to_null_terminated(words);
  vec_destroy(words);

  n->cmd.nredirs = redirs->count;
  if (redirs->count > 0) {
    n->cmd.redirs = mem_alloc((size_t)redirs->count * sizeof(Redir));
    for (int i = 0; i < redirs->count; i++) {
      n->cmd.redirs[i] = *(Redir *)redirs->data[i];
      mem_free(redirs->data[i]);
    }
  }
  vec_destroy(redirs);
  return n;
}

/* ------------------------------------------------------------------ */
/* Block: { stmts }                                                    */
/* ------------------------------------------------------------------ */

static AstNode *parse_block(Parser *p);   /* forward */
static AstNode *parse_command(Parser *p); /* forward */
static AstNode *parse_and_or(Parser *p);  /* forward */

/* Parse a list of statements until we see } or EOF. */
static AstNode *parse_stmt_list_until_rbrace(Parser *p) {
  Vec *items = vec_create();
  skip_newlines(p);
  while (!at(p, TOK_RBRACE) && !at(p, TOK_EOF)) {
    if (p->error)
      break;
    AstNode *stmt = parse_and_or(p);
    if (stmt)
      vec_push(items, stmt);

    /* Consume statement terminator */
    Token *t = peek(p);
    if (t->type == TOK_SEMI || t->type == TOK_NEWLINE || t->type == TOK_AMP) {
      int bg = t->type == TOK_AMP;
      token_destroy(advance(p));
      if (bg && items->count > 0) {
        AstNode *last = items->data[items->count - 1];
        AstNode *bgn = ast_node_new(NODE_BG);
        bgn->bg.child = last;
        items->data[items->count - 1] = bgn;
      }
    }
    skip_newlines(p);
  }

  if (items->count == 0) {
    vec_destroy(items);
    return ast_node_new(NODE_LIST); /* empty list */
  }
  if (items->count == 1) {
    AstNode *single = items->data[0];
    vec_destroy(items);
    return single;
  }
  AstNode *n = ast_node_new(NODE_LIST);
  n->list.count = items->count;
  n->list.items = (AstNode **)vec_to_null_terminated(items);
  vec_destroy(items);
  return n;
}

static AstNode *parse_block(Parser *p) {
  if (!expect(p, TOK_LBRACE)) {
    p->incomplete = 1;
    return NULL;
  }
  skip_newlines(p);
  AstNode *body = parse_stmt_list_until_rbrace(p);
  if (!expect(p, TOK_RBRACE)) {
    p->incomplete = 1;
    ast_free(body);
    return NULL;
  }
  return body;
}

/* ------------------------------------------------------------------ */
/* if / while / for / def                                              */
/* ------------------------------------------------------------------ */

static AstNode *parse_if(Parser *p) {
  /* "if" already consumed by caller */
  Vec *clauses = vec_create();
  int done = 0;

  while (!done) {
    /* Parse condition pipeline */
    AstNode *cond = parse_and_or(p);
    if (!cond || p->error) {
      done = 1;
      break;
    }

    skip_newlines(p);
    AstNode *body = parse_block(p);
    if (!body || p->error) {
      ast_free(cond);
      done = 1;
      break;
    }

    IfClause *cl = mem_alloc(sizeof(IfClause));
    cl->cond = cond;
    cl->body = body;
    vec_push(clauses, cl);

    skip_newlines(p);
    if (is_keyword(p, "elif")) {
      token_destroy(advance(p));
      skip_newlines(p);
    } else {
      done = 1;
    }
  }

  AstNode *else_body = NULL;
  skip_newlines(p);
  if (is_keyword(p, "else")) {
    token_destroy(advance(p));
    skip_newlines(p);
    else_body = parse_block(p);
  }

  AstNode *n = ast_node_new(NODE_IF);
  n->ifstmt.nclause = clauses->count;
  n->ifstmt.clauses = mem_alloc((size_t)clauses->count * sizeof(IfClause));
  for (int i = 0; i < clauses->count; i++) {
    n->ifstmt.clauses[i] = *(IfClause *)clauses->data[i];
    mem_free(clauses->data[i]);
  }
  vec_destroy(clauses);
  n->ifstmt.else_body = else_body;
  return n;
}

static AstNode *parse_while(Parser *p) {
  /* "while" already consumed */
  AstNode *cond = parse_and_or(p);
  if (!cond || p->error)
    return NULL;
  skip_newlines(p);
  AstNode *body = parse_block(p);
  if (!body || p->error) {
    ast_free(cond);
    return NULL;
  }
  AstNode *n = ast_node_new(NODE_WHILE);
  n->whilestmt.cond = cond;
  n->whilestmt.body = body;
  return n;
}

static AstNode *parse_for(Parser *p) {
  /* "for" already consumed */
  if (!at(p, TOK_WORD)) {
    fprintf(stderr, "bang-lish: for: expected variable name\n");
    p->error = 1;
    return NULL;
  }
  Token *var_tok = advance(p);
  char *var = mem_strdup(var_tok->value);
  token_destroy(var_tok);

  if (!is_keyword(p, "in")) {
    fprintf(stderr, "bang-lish: for: expected 'in'\n");
    mem_free(var);
    p->error = 1;
    return NULL;
  }
  token_destroy(advance(p)); /* consume "in" */

  /* Collect word list until { */
  Vec *words = vec_create();
  while (at(p, TOK_WORD)) {
    Token *wt = advance(p);
    vec_push(words, mem_strdup(wt->value));
    token_destroy(wt);
  }

  skip_newlines(p);
  AstNode *body = parse_block(p);
  if (!body || p->error) {
    mem_free(var);
    vec_destroy_items(words, mem_free);
    return NULL;
  }

  AstNode *n = ast_node_new(NODE_FOR);
  n->forstmt.var = var;
  n->forstmt.words = (char **)vec_to_null_terminated(words);
  vec_destroy(words);
  n->forstmt.body = body;
  return n;
}

static AstNode *parse_def(Parser *p) {
  /* "def" already consumed */
  if (!at(p, TOK_WORD)) {
    fprintf(stderr, "bang-lish: def: expected function name\n");
    p->error = 1;
    return NULL;
  }
  Token *name_tok = advance(p);
  char *name = mem_strdup(name_tok->value);
  token_destroy(name_tok);

  if (!expect(p, TOK_LPAREN)) {
    mem_free(name);
    return NULL;
  }

  /* Collect parameter names until ) */
  Vec *params = vec_create();
  while (at(p, TOK_WORD)) {
    Token *pt = advance(p);
    vec_push(params, mem_strdup(pt->value));
    token_destroy(pt);
  }

  if (!expect(p, TOK_RPAREN)) {
    mem_free(name);
    vec_destroy_items(params, mem_free);
    return NULL;
  }

  skip_newlines(p);
  AstNode *body = parse_block(p);
  if (!body || p->error) {
    mem_free(name);
    vec_destroy_items(params, mem_free);
    return NULL;
  }

  AstNode *n = ast_node_new(NODE_DEF);
  n->def.name = name;
  n->def.params = (char **)vec_to_null_terminated(params);
  vec_destroy(params);
  n->def.body = body;
  return n;
}

/* ------------------------------------------------------------------ */
/* Command dispatch                                                    */
/* ------------------------------------------------------------------ */

static AstNode *parse_command(Parser *p) {
  /* Check for bang-lish compound commands / declarations */
  if (is_keyword(p, "if")) {
    token_destroy(advance(p));
    return parse_if(p);
  }
  if (is_keyword(p, "while")) {
    token_destroy(advance(p));
    return parse_while(p);
  }
  if (is_keyword(p, "for")) {
    token_destroy(advance(p));
    return parse_for(p);
  }
  if (is_keyword(p, "def")) {
    token_destroy(advance(p));
    return parse_def(p);
  }
  /* Special: "let NAME = VALUE" */
  if (is_keyword(p, "let")) {
    token_destroy(advance(p)); /* consume "let" */
    if (!at(p, TOK_WORD)) {
      fprintf(stderr, "bang-lish: let: expected variable name\n");
      p->error = 1;
      return NULL;
    }
    Token *name_tok = advance(p);
    char *name = mem_strdup(name_tok->value);
    token_destroy(name_tok);

    /* Expect "=" as a word token */
    if (!at(p, TOK_WORD) || !peek(p)->value ||
        strcmp(peek(p)->value, "=") != 0) {
      fprintf(stderr, "bang-lish: let: expected '='\n");
      mem_free(name);
      p->error = 1;
      return NULL;
    }
    token_destroy(advance(p)); /* consume "=" */

    /* Collect the value (may be multiple words — join with space) */
    Vec *val_parts = vec_create();
    while (at(p, TOK_WORD)) {
      Token *vt = advance(p);
      vec_push(val_parts, mem_strdup(vt->value));
      token_destroy(vt);
    }
    char *value;
    if (val_parts->count == 0) {
      value = mem_strdup("");
    } else if (val_parts->count == 1) {
      value = (char *)val_parts->data[0];
    } else {
      value = str_join((char **)val_parts->data, " ");
      for (int i = 0; i < val_parts->count; i++)
        mem_free(val_parts->data[i]);
    }
    vec_destroy(val_parts);

    AstNode *n = ast_node_new(NODE_LET);
    n->let.name = name;
    n->let.value = value;
    return n;
  }
  /* Special: "export NAME" */
  if (is_keyword(p, "export")) {
    token_destroy(advance(p));
    if (!at(p, TOK_WORD)) {
      fprintf(stderr, "bang-lish: export: expected variable name\n");
      p->error = 1;
      return NULL;
    }
    Token *nt = advance(p);
    AstNode *n = ast_node_new(NODE_EXPORT);
    n->export_.name = mem_strdup(nt->value);
    token_destroy(nt);
    return n;
  }

  return parse_simple_cmd(p);
}

/* ------------------------------------------------------------------ */
/* Pipeline and and/or                                                 */
/* ------------------------------------------------------------------ */

static AstNode *parse_pipeline(Parser *p) {
  Vec *cmds = vec_create();

  AstNode *first = parse_command(p);
  if (!first) {
    vec_destroy(cmds);
    return NULL;
  }
  vec_push(cmds, first);

  while (at(p, TOK_PIPE) || at(p, TOK_PIPE_ERR)) {
    int is_pipe_err = at(p, TOK_PIPE_ERR);
    token_destroy(advance(p));
    skip_newlines(p);
    AstNode *next = parse_command(p);
    if (!next || p->error)
      break;

    if (is_pipe_err) {
      /* For |&: redirect stderr to stdout on the preceding stage */
      AstNode *prev = cmds->data[cmds->count - 1];
      if (prev->kind == NODE_CMD) {
        prev->cmd.redirs = mem_realloc(
            prev->cmd.redirs, (size_t)(prev->cmd.nredirs + 1) * sizeof(Redir));
        Redir *r = &prev->cmd.redirs[prev->cmd.nredirs++];
        r->type = REDIR_ERR;
        r->fd = 2;
        r->target = mem_strdup("&1"); /* signal to eval to dup stderr→stdout */
      }
    }
    vec_push(cmds, next);
  }

  if (cmds->count == 1) {
    AstNode *single = cmds->data[0];
    vec_destroy(cmds);
    return single;
  }

  AstNode *n = ast_node_new(NODE_PIPE);
  n->pipe.count = cmds->count;
  n->pipe.cmds = (AstNode **)vec_to_null_terminated(cmds);
  vec_destroy(cmds);
  return n;
}

static AstNode *parse_and_or(Parser *p) {
  AstNode *left = parse_pipeline(p);
  if (!left)
    return NULL;

  while (at(p, TOK_AND) || at(p, TOK_OR)) {
    NodeKind kind = at(p, TOK_AND) ? NODE_AND : NODE_OR;
    token_destroy(advance(p));
    skip_newlines(p);
    AstNode *right = parse_pipeline(p);
    if (!right || p->error)
      break;

    AstNode *n = ast_node_new(kind);
    n->binary.left = left;
    n->binary.right = right;
    left = n;
  }
  return left;
}

/* ------------------------------------------------------------------ */
/* Statement                                                           */
/* ------------------------------------------------------------------ */

static AstNode *parse_statement(Parser *p) {
  skip_newlines(p);

  Token *t = peek(p);
  if (t->type == TOK_EOF)
    return NULL;

  AstNode *node = parse_and_or(p);
  if (!node)
    return NULL;

  /* Check for & (background) */
  if (at(p, TOK_AMP)) {
    token_destroy(advance(p));
    AstNode *bg = ast_node_new(NODE_BG);
    bg->bg.child = node;
    node = bg;
  }

  return node;
}

/* ------------------------------------------------------------------ */
/* Public API                                                          */
/* ------------------------------------------------------------------ */

Parser *parser_create(Lexer *lx, struct ShellState *sh) {
  Parser *p = mem_alloc(sizeof(Parser));
  p->lx = lx;
  p->sh = sh;
  p->error = 0;
  p->incomplete = 0;
  return p;
}

void parser_destroy(Parser *p) { mem_free(p); }

ParseResult parse_line(Parser *p, AstNode **out_node) {
  *out_node = NULL;
  p->error = 0;
  p->incomplete = 0;

  skip_newlines(p);
  if (at(p, TOK_EOF))
    return PARSE_EOF;

  AstNode *node = parse_statement(p);

  if (p->incomplete)
    return PARSE_INCOMPLETE;
  if (p->error)
    return PARSE_ERROR;
  if (!node)
    return PARSE_EOF;

  *out_node = node;
  return PARSE_OK;
}

AstNode *parse_script(Parser *p) {
  Vec *items = vec_create();

  while (!at(p, TOK_EOF) && !p->error) {
    skip_newlines(p);
    if (at(p, TOK_EOF))
      break;

    AstNode *stmt = parse_statement(p);
    if (!stmt)
      break;
    vec_push(items, stmt);

    Token *t = peek(p);
    if (t->type == TOK_SEMI || t->type == TOK_NEWLINE) {
      token_destroy(advance(p));
    }
  }

  if (items->count == 0) {
    vec_destroy(items);
    return NULL;
  }
  if (items->count == 1) {
    AstNode *s = items->data[0];
    vec_destroy(items);
    return s;
  }

  AstNode *root = ast_node_new(NODE_LIST);
  root->list.count = items->count;
  root->list.items = (AstNode **)vec_to_null_terminated(items);
  vec_destroy(items);
  return root;
}
