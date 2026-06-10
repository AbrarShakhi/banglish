#pragma once

typedef struct AstNode AstNode;

typedef enum {
  REDIR_IN,          /* <        */
  REDIR_OUT,         /* >        */
  REDIR_APPEND,      /* >>       */
  REDIR_ERR,         /* 2>       */
  REDIR_ERR_APPEND,  /* 2>>      */
  REDIR_BOTH,        /* &>       */
  REDIR_BOTH_APPEND, /* &>>      */
  REDIR_HEREDOC,     /* <<       */
} RedirType;

typedef struct {
  RedirType type;
  int fd;       /* source fd (0/1/2); -1 for REDIR_BOTH */
  char *target; /* filename or heredoc delimiter; heap-owned */
} Redir;

typedef struct {
  AstNode *cond;
  AstNode *body;
} IfClause;

typedef enum {
  NODE_LIST,   /* sequence of statements: items[] */
  NODE_AND,    /* left && right                   */
  NODE_OR,     /* left || right                   */
  NODE_PIPE,   /* pipeline: cmds[]                */
  NODE_CMD,    /* simple command: argv[], redirs[] */
  NODE_BG,     /* background: child               */
  NODE_IF,     /* if/elif/else                    */
  NODE_WHILE,  /* while cond { body }             */
  NODE_FOR,    /* for var in words { body }       */
  NODE_DEF,    /* def name(params) { body }       */
  NODE_LET,    /* let name = value                */
  NODE_EXPORT, /* export name                     */
} NodeKind;

struct AstNode {
  NodeKind kind;
  union {
    /* NODE_LIST */
    struct {
      AstNode **items;
      int count;
    } list;

    /* NODE_AND, NODE_OR */
    struct {
      AstNode *left;
      AstNode *right;
    } binary;

    /* NODE_PIPE */
    struct {
      AstNode **cmds;
      int count;
    } pipe;

    /* NODE_CMD */
    struct {
      char **argv; /* NULL-terminated; heap-owned strings */
      int argc;
      Redir *redirs; /* array; heap-owned */
      int nredirs;
    } cmd;

    /* NODE_BG */
    struct {
      AstNode *child;
    } bg;

    /* NODE_IF */
    struct {
      IfClause *clauses; /* if + elif clauses */
      int nclause;
      AstNode *else_body;
    } ifstmt;

    /* NODE_WHILE */
    struct {
      AstNode *cond;
      AstNode *body;
    } whilestmt;

    /* NODE_FOR */
    struct {
      char *var;    /* loop variable name; heap-owned */
      char **words; /* NULL-terminated word list; each heap-owned */
      AstNode *body;
    } forstmt;

    /* NODE_DEF */
    struct {
      char *name;    /* heap-owned */
      char **params; /* NULL-terminated; each heap-owned */
      AstNode *body;
    } def;

    /* NODE_LET */
    struct {
      char *name;
      char *value;
    } let; /* both heap-owned */

    /* NODE_EXPORT */
    struct {
      char *name;
    } export_; /* heap-owned */
  };
};

AstNode *ast_node_new(NodeKind kind);
void ast_free(AstNode *node);
void ast_dump(const AstNode *node, int indent); /* debug */
