#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "core/shell.h"
#include "core/signal.h"
#include "exec/eval.h"
#include "exec/job.h"
#include "lexer/lexer.h"
#include "parser/parser.h"
#include "prompt/prompt.h"
#include "utils/memory.h"

/* ------------------------------------------------------------------ */
/* Interactive REPL                                                    */
/* ------------------------------------------------------------------ */

static void notify_done_jobs(ShellState *sh) {
  job_update_status(sh->jobs);
  Job **pp = &sh->jobs->head;
  while (*pp) {
    Job *j = *pp;
    if (j->status == JOB_DONE && !j->notified) {
      fprintf(stderr, "[%d]+ Done\t%s\n", j->jid, j->cmdline);
      j->notified = 1;
    }
    if (j->status == JOB_DONE && j->notified) {
      *pp = j->next;
      mem_free(j->cmdline);
      mem_free(j);
    } else
      pp = &(*pp)->next;
  }
}

static void repl(ShellState *sh) {
  char *accum = NULL;
  size_t accum_len = 0;

  char *line;
  while ((line = prompt_readline(sh)) != NULL) {
    notify_done_jobs(sh);

    if (accum) {
      size_t old = accum_len;
      accum_len = old + strlen(line) + 2;
      accum = mem_realloc(accum, accum_len);
      accum[old] = '\n';
      strcpy(accum + old + 1, line);
    } else {
      accum = mem_strdup(line);
      accum_len = strlen(accum) + 1;
    }
    free(line); /* readline-allocated; free with free(), not mem_free() */

    /* Check if input is complete or needs more lines */
    Lexer *lx = lexer_create(accum);
    Parser *p = parser_create(lx, sh);
    AstNode *node = NULL;
    ParseResult res = parse_line(p, &node);
    if (node)
      ast_free(node);
    parser_destroy(p);
    lexer_destroy(lx);

    if (res == PARSE_INCOMPLETE) {
      sh->secondary_prompt = 1;
      continue;
    }
    sh->secondary_prompt = 0;

    /* Full parse + execute all statements in buffer */
    Lexer *lx2 = lexer_create(accum);
    Parser *p2 = parser_create(lx2, sh);
    AstNode *tree = parse_script(p2);
    parser_destroy(p2);
    lexer_destroy(lx2);

    if (tree) {
      eval_node(sh, tree);
      ast_free(tree);
    }

    mem_free(accum);
    accum = NULL;
    accum_len = 0;
  }

  if (accum)
    mem_free(accum);
  if (sh->interactive)
    fputs("\nexit\n", stderr);
}

/* ------------------------------------------------------------------ */
/* main                                                                */
/* ------------------------------------------------------------------ */

int main(int argc, char **argv) {
  ShellState *sh = shell_init(argc, argv);

  int arg_idx = 1;
  int run_cmd = 0;

  while (arg_idx < argc && argv[arg_idx][0] == '-') {
    if (strcmp(argv[arg_idx], "-c") == 0) {
      run_cmd = 1;
      arg_idx++;
      break;
    }
    if (strcmp(argv[arg_idx], "--") == 0) {
      arg_idx++;
      break;
    }
    arg_idx++;
  }

  if (run_cmd) {
    if (arg_idx >= argc) {
      fprintf(stderr, "banglish: -c: argument required\n");
      shell_destroy(sh);
      return 1;
    }
    sh->script_argc = argc - arg_idx;
    sh->script_argv = argv + arg_idx;
    int status = eval_string(sh, argv[arg_idx]);
    shell_destroy(sh);
    return status;
  }

  if (arg_idx < argc) {
    /* Script file mode */
    sh->interactive = 0;
    sh->script_argc = argc - arg_idx;
    sh->script_argv = argv + arg_idx;
    signals_init_script();
    int status = eval_file(sh, argv[arg_idx]);
    shell_destroy(sh);
    return status;
  }

  /* Interactive or piped stdin */
  sh->interactive = isatty(STDIN_FILENO);

  if (sh->interactive) {
    signals_init_interactive();
    prompt_init(sh);

    const char *home = getenv("HOME");
    if (home) {
      char rc[PATH_MAX];
      snprintf(rc, sizeof(rc), "%s/.banglishrc", home);
      if (access(rc, F_OK) == 0)
        eval_file(sh, rc);
    }

    repl(sh);
    prompt_teardown(sh);
  } else {
    signals_init_script();
    char *buf = NULL;
    size_t cap = 0, len = 0;
    char tmp[4096];
    ssize_t nr;
    while ((nr = read(STDIN_FILENO, tmp, sizeof(tmp))) > 0) {
      if (len + (size_t)nr + 1 > cap) {
        cap = (cap + (size_t)nr + 1024) * 2;
        buf = mem_realloc(buf, cap);
      }
      memcpy(buf + len, tmp, (size_t)nr);
      len += (size_t)nr;
    }
    if (buf) {
      buf[len] = '\0';
      eval_string(sh, buf);
      mem_free(buf);
    }
  }

  int final = sh->last_status;
  shell_destroy(sh);
  return final;
}
