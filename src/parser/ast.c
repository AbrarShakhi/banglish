#include "ast.h"
#include "utils/memory.h"
#include <stdio.h>
#include <string.h>

AstNode *ast_node_new(NodeKind kind) {
  AstNode *n = mem_alloc(sizeof(AstNode));
  memset(n, 0, sizeof(AstNode));
  n->kind = kind;
  return n;
}

static void free_redir_array(Redir *redirs, int n) {
  if (!redirs)
    return;
  for (int i = 0; i < n; i++)
    mem_free(redirs[i].target);
  mem_free(redirs);
}

static void free_str_array(char **arr) {
  if (!arr)
    return;
  for (int i = 0; arr[i]; i++)
    mem_free(arr[i]);
  mem_free(arr);
}

void ast_free(AstNode *node) {
  if (!node)
    return;
  switch (node->kind) {
  case NODE_LIST:
    for (int i = 0; i < node->list.count; i++)
      ast_free(node->list.items[i]);
    mem_free(node->list.items);
    break;
  case NODE_AND:
  case NODE_OR:
    ast_free(node->binary.left);
    ast_free(node->binary.right);
    break;
  case NODE_PIPE:
    for (int i = 0; i < node->pipe.count; i++)
      ast_free(node->pipe.cmds[i]);
    mem_free(node->pipe.cmds);
    break;
  case NODE_CMD:
    free_str_array(node->cmd.argv);
    free_redir_array(node->cmd.redirs, node->cmd.nredirs);
    break;
  case NODE_BG:
    ast_free(node->bg.child);
    break;
  case NODE_IF:
    for (int i = 0; i < node->ifstmt.nclause; i++) {
      ast_free(node->ifstmt.clauses[i].cond);
      ast_free(node->ifstmt.clauses[i].body);
    }
    mem_free(node->ifstmt.clauses);
    ast_free(node->ifstmt.else_body);
    break;
  case NODE_WHILE:
    ast_free(node->whilestmt.cond);
    ast_free(node->whilestmt.body);
    break;
  case NODE_FOR:
    mem_free(node->forstmt.var);
    free_str_array(node->forstmt.words);
    ast_free(node->forstmt.body);
    break;
  case NODE_DEF:
    mem_free(node->def.name);
    free_str_array(node->def.params);
    ast_free(node->def.body);
    break;
  case NODE_LET:
    mem_free(node->let.name);
    mem_free(node->let.value);
    break;
  case NODE_EXPORT:
    mem_free(node->export_.name);
    break;
  }
  mem_free(node);
}

static const char *kind_name(NodeKind k) {
  switch (k) {
  case NODE_LIST:
    return "LIST";
  case NODE_AND:
    return "AND";
  case NODE_OR:
    return "OR";
  case NODE_PIPE:
    return "PIPE";
  case NODE_CMD:
    return "CMD";
  case NODE_BG:
    return "BG";
  case NODE_IF:
    return "IF";
  case NODE_WHILE:
    return "WHILE";
  case NODE_FOR:
    return "FOR";
  case NODE_DEF:
    return "DEF";
  case NODE_LET:
    return "LET";
  case NODE_EXPORT:
    return "EXPORT";
  default:
    return "?";
  }
}

void ast_dump(const AstNode *node, int indent) {
  if (!node)
    return;
  for (int i = 0; i < indent; i++)
    fputs("  ", stderr);
  fprintf(stderr, "[%s]", kind_name(node->kind));
  switch (node->kind) {
  case NODE_CMD:
    for (int i = 0; i < node->cmd.argc; i++)
      fprintf(stderr, " %s", node->cmd.argv[i]);
    break;
  case NODE_LET:
    fprintf(stderr, " %s = %s", node->let.name, node->let.value);
    break;
  case NODE_EXPORT:
    fprintf(stderr, " %s", node->export_.name);
    break;
  case NODE_DEF:
    fprintf(stderr, " %s", node->def.name);
    break;
  case NODE_FOR:
    fprintf(stderr, " %s in ...", node->forstmt.var);
    break;
  default:
    break;
  }
  fputc('\n', stderr);
  switch (node->kind) {
  case NODE_LIST:
    for (int i = 0; i < node->list.count; i++)
      ast_dump(node->list.items[i], indent + 1);
    break;
  case NODE_AND:
  case NODE_OR:
    ast_dump(node->binary.left, indent + 1);
    ast_dump(node->binary.right, indent + 1);
    break;
  case NODE_PIPE:
    for (int i = 0; i < node->pipe.count; i++)
      ast_dump(node->pipe.cmds[i], indent + 1);
    break;
  case NODE_BG:
    ast_dump(node->bg.child, indent + 1);
    break;
  case NODE_WHILE:
    ast_dump(node->whilestmt.cond, indent + 1);
    ast_dump(node->whilestmt.body, indent + 1);
    break;
  case NODE_FOR:
    ast_dump(node->forstmt.body, indent + 1);
    break;
  case NODE_DEF:
    ast_dump(node->def.body, indent + 1);
    break;
  case NODE_IF:
    for (int i = 0; i < node->ifstmt.nclause; i++) {
      ast_dump(node->ifstmt.clauses[i].cond, indent + 1);
      ast_dump(node->ifstmt.clauses[i].body, indent + 1);
    }
    ast_dump(node->ifstmt.else_body, indent + 1);
    break;
  default:
    break;
  }
}
