#include "eval.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/wait.h>
#include <unistd.h>

#include "core/env.h"
#include "expand/expand.h"
#include "job.h"
#include "lexer/lexer.h"
#include "parser/parser.h"
#include "pipeline.h"
#include "scripting/funcstore.h"
#include "utils/memory.h"
#include "utils/str.h"
#include "utils/vec.h"

/* ------------------------------------------------------------------ */
/* eval_node: walk the AST and execute                                 */
/* ------------------------------------------------------------------ */

int eval_node(ShellState *sh, AstNode *node) {
  if (!node)
    return 0;

  switch (node->kind) {
  /* ---- List (sequence) ---- */
  case NODE_LIST: {
    int status = 0;
    for (int i = 0; i < node->list.count; i++)
      status = eval_node(sh, node->list.items[i]);
    sh->last_status = status;
    return status;
  }

  /* ---- && / || ---- */
  case NODE_AND: {
    int left = eval_node(sh, node->binary.left);
    sh->last_status = left;
    if (left != 0)
      return left;
    int right = eval_node(sh, node->binary.right);
    sh->last_status = right;
    return right;
  }
  case NODE_OR: {
    int left = eval_node(sh, node->binary.left);
    sh->last_status = left;
    if (left == 0)
      return left;
    int right = eval_node(sh, node->binary.right);
    sh->last_status = right;
    return right;
  }

  /* ---- Pipeline ---- */
  case NODE_PIPE: {
    int status = exec_pipeline(sh, node);
    sh->last_status = status;
    return status;
  }

  /* ---- Simple command ---- */
  case NODE_CMD: {
    if (node->cmd.argc == 0)
      return 0;
    char **xargv = argv_expand(node->cmd.argv, node->cmd.argc, sh);
    int status = exec_simple_cmd(sh, node, xargv);
    expand_free_words(xargv);
    sh->last_status = status;
    return status;
  }

  /* ---- Background ---- */
  case NODE_BG: {
    AstNode *child = node->bg.child;
    /* Build a command string for the job table */
    char *cmdline = mem_strdup("<background>");
    if (child->kind == NODE_CMD && child->cmd.argc > 0)
      cmdline = str_join(child->cmd.argv, " ");
    else if (child->kind == NODE_PIPE && child->pipe.count > 0) {
      AstNode *first = child->pipe.cmds[0];
      if (first->kind == NODE_CMD)
        cmdline = str_join(first->cmd.argv, " ");
    }

    pid_t pid = fork();
    if (pid < 0) {
      perror("bang-lish: fork");
      mem_free(cmdline);
      return 1;
    }
    if (pid == 0) {
      /* Child: run without waiting */
      setpgid(0, 0);
      int ret = eval_node(sh, child);
      exit(ret);
    }
    setpgid(pid, pid);
    Job *j = job_add(sh->jobs, pid, cmdline);
    mem_free(cmdline);
    fprintf(stderr, "[%d] %d\n", j->jid, (int)pid);
    sh->last_status = 0;
    return 0;
  }

  /* ---- if / elif / else ---- */
  case NODE_IF: {
    for (int i = 0; i < node->ifstmt.nclause; i++) {
      int cond = eval_node(sh, node->ifstmt.clauses[i].cond);
      if (cond == 0) {
        int ret = eval_node(sh, node->ifstmt.clauses[i].body);
        sh->last_status = ret;
        return ret;
      }
    }
    if (node->ifstmt.else_body) {
      int ret = eval_node(sh, node->ifstmt.else_body);
      sh->last_status = ret;
      return ret;
    }
    return 0;
  }

  /* ---- while ---- */
  case NODE_WHILE: {
    int status = 0;
    while (eval_node(sh, node->whilestmt.cond) == 0) {
      status = eval_node(sh, node->whilestmt.body);
      sh->last_status = status;
    }
    return status;
  }

  /* ---- for ---- */
  case NODE_FOR: {
    int status = 0;
    /* Expand all words in the word list */
    Vec *expanded = vec_create();
    for (int i = 0; node->forstmt.words[i]; i++) {
      char **ws = word_expand(node->forstmt.words[i], sh);
      for (int k = 0; ws[k]; k++)
        vec_push(expanded, ws[k]);
      mem_free(ws);
    }
    for (int i = 0; i < expanded->count; i++) {
      const char *word = (char *)expanded->data[i];
      env_set(sh->env, node->forstmt.var, word);
      status = eval_node(sh, node->forstmt.body);
      sh->last_status = status;
    }
    vec_destroy_items(expanded, mem_free);
    return status;
  }

  /* ---- let: shell variable assignment ---- */
  case NODE_LET: {
    char *val = expand_variables_only(node->let.value, sh);
    env_set(sh->env, node->let.name, val);
    mem_free(val);
    sh->last_status = 0;
    return 0;
  }

  /* ---- export ---- */
  case NODE_EXPORT: {
    env_export(sh->env, node->export_.name);
    sh->last_status = 0;
    return 0;
  }
  }
  return 0;
}

/* ------------------------------------------------------------------ */
/* eval_string / eval_file                                             */
/* ------------------------------------------------------------------ */

int eval_string(ShellState *sh, const char *src) {
  if (!src || !*src)
    return 0;
  Lexer *lx = lexer_create(src);
  Parser *p = parser_create(lx, sh);
  AstNode *tree = parse_script(p);
  parser_destroy(p);
  lexer_destroy(lx);

  int status = 0;
  if (tree) {
    status = eval_node(sh, tree);
    ast_free(tree);
  }
  return status;
}

int eval_file(ShellState *sh, const char *path) {
  FILE *f = fopen(path, "r");
  if (!f) {
    perror(path);
    return 1;
  }
  fseek(f, 0, SEEK_END);
  long fsize = ftell(f);
  rewind(f);
  if (fsize < 0) {
    fclose(f);
    return 1;
  }

  char *buf = mem_alloc((size_t)fsize + 1);
  size_t nr = fread(buf, 1, (size_t)fsize, f);
  buf[nr] = '\0';
  fclose(f);

  /* Skip shebang line */
  const char *src = buf;
  if (src[0] == '#' && src[1] == '!') {
    while (*src && *src != '\n')
      src++;
    if (*src == '\n')
      src++;
  }

  int status = eval_string(sh, src);
  mem_free(buf);
  return status;
}
