#include "pipeline.h"
#include "builtin/alias.h"
#include "builtin/registry.h"
#include "core/env.h"
#include "core/signal.h"
#include "eval.h"
#include "expand/expand.h"
#include "job.h"
#include "redirect.h"
#include "scripting/funcstore.h"
#include "utils/memory.h"
#include "utils/str.h"
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/wait.h>
#include <unistd.h>

static void give_terminal_to(pid_t pgid) {
  /* Suppress SIGTTOU so tcsetpgrp doesn't stop us */
  signal(SIGTTOU, SIG_IGN);
  tcsetpgrp(STDIN_FILENO, pgid);
  signal(SIGTTOU, SIG_IGN);
}

/* Execute a simple command (expanded argv already provided).
   Returns exit status. */
int exec_simple_cmd(ShellState *sh, const AstNode *cmd_node,
                    char **expanded_argv) {
  if (!expanded_argv || !expanded_argv[0])
    return 0;

  /* Alias expansion */
  if (sh->aliases) {
    const char *alias_val =
        aliastable_get((AliasTable *)sh->aliases, expanded_argv[0]);
    if (alias_val) {
      /* Replace argv[0] with alias value (simple substitution) */
      char *new_argv0 = mem_strdup(alias_val);
      mem_free(expanded_argv[0]);
      expanded_argv[0] = new_argv0;
    }
  }

  /* Check builtins */
  BuiltinFn fn = builtin_find(expanded_argv[0]);
  if (fn) {
    RedirSave save;
    int has_redirs = cmd_node && cmd_node->cmd.nredirs > 0;
    if (has_redirs &&
        redirects_apply(cmd_node->cmd.redirs, cmd_node->cmd.nredirs, &save) < 0)
      return 1;
    int argc = str_array_len(expanded_argv);
    int ret = fn(sh, argc, expanded_argv);
    if (has_redirs)
      redirects_restore(&save);
    return ret;
  }

  /* Check user-defined functions */
  if (sh->funcs) {
    FuncEntry *fe = functable_find((FuncTable *)sh->funcs, expanded_argv[0]);
    if (fe) {
      int saved_argc = sh->script_argc;
      char **saved_argv = sh->script_argv;
      int nargs = str_array_len(expanded_argv);
      sh->script_argc = nargs;
      sh->script_argv = expanded_argv;

      int nparam = str_array_len(fe->params);
      for (int i = 0; i < nparam; i++) {
        const char *val = (i + 1 < nargs) ? expanded_argv[i + 1] : "";
        env_set(sh->env, fe->params[i], val);
      }

      int ret = eval_node(sh, fe->body);
      sh->script_argc = saved_argc;
      sh->script_argv = saved_argv;
      return ret;
    }
  }

  /* Fork and exec external command */
  fflush(NULL);
  pid_t pid = fork();
  if (pid < 0) {
    perror("bang-lish: fork");
    return 1;
  }

  if (pid == 0) {
    /* Child */
    setpgid(0, 0);
    signals_init_child();
    if (sh->interactive)
      give_terminal_to(getpid());

    if (cmd_node && cmd_node->cmd.nredirs > 0) {
      if (redirects_apply(cmd_node->cmd.redirs, cmd_node->cmd.nredirs, NULL) <
          0)
        exit(1);
    }
    execvp(expanded_argv[0], expanded_argv);
    if (errno == ENOENT)
      fprintf(stderr, "bang-lish: %s: command not found\n", expanded_argv[0]);
    else
      perror(expanded_argv[0]);
    exit(127);
  }

  /* Parent */
  setpgid(pid, pid);
  if (sh->interactive)
    give_terminal_to(pid);

  int wstatus;
  waitpid(pid, &wstatus, WUNTRACED);

  /* Reclaim terminal */
  if (sh->interactive)
    give_terminal_to(getpgrp());

  if (WIFSTOPPED(wstatus)) {
    char *cmdline = str_join(expanded_argv, " ");
    Job *j = job_add(sh->jobs, pid, cmdline);
    mem_free(cmdline);
    j->status = JOB_STOPPED;
    fprintf(stderr, "\n[%d]+ Stopped\t%s\n", j->jid, j->cmdline);
    return 128 + WSTOPSIG(wstatus);
  }
  if (WIFSIGNALED(wstatus)) {
    if (WTERMSIG(wstatus) == SIGINT)
      fprintf(stderr, "\n");
    return 128 + WTERMSIG(wstatus);
  }
  return WIFEXITED(wstatus) ? WEXITSTATUS(wstatus) : 1;
}

/* Execute a pipeline: cmd1 | cmd2 | ... */
int exec_pipeline(ShellState *sh, AstNode *pipe_node) {
  int n = pipe_node->pipe.count;
  int(*pipes)[2] = mem_alloc((size_t)(n - 1) * sizeof(int[2]));
  pid_t *pids = mem_alloc((size_t)n * sizeof(pid_t));

  for (int i = 0; i < n - 1; i++) {
    if (pipe(pipes[i]) < 0) {
      perror("bang-lish: pipe");
      mem_free(pipes);
      mem_free(pids);
      return 1;
    }
  }

  pid_t pgid = 0;

  for (int i = 0; i < n; i++) {
    AstNode *stage = pipe_node->pipe.cmds[i];
    char **xargv = NULL;
    if (stage->kind == NODE_CMD)
      xargv = argv_expand(stage->cmd.argv, stage->cmd.argc, sh);

    fflush(NULL);
    pid_t pid = fork();
    if (pid < 0) {
      perror("bang-lish: fork");
      for (int k = 0; k < n - 1; k++) {
        close(pipes[k][0]);
        close(pipes[k][1]);
      }
      mem_free(pipes);
      mem_free(pids);
      expand_free_words(xargv);
      return 1;
    }

    if (pid == 0) {
      if (pgid == 0)
        pgid = getpid();
      setpgid(0, pgid);
      signals_init_child();

      if (i > 0)
        dup2(pipes[i - 1][0], STDIN_FILENO);
      if (i < n - 1)
        dup2(pipes[i][1], STDOUT_FILENO);

      for (int k = 0; k < n - 1; k++) {
        close(pipes[k][0]);
        close(pipes[k][1]);
      }

      if (stage->kind == NODE_CMD && stage->cmd.nredirs > 0)
        redirects_apply(stage->cmd.redirs, stage->cmd.nredirs, NULL);

      if (xargv && xargv[0]) {
        BuiltinFn fn = builtin_find(xargv[0]);
        if (fn) {
          int argc = str_array_len(xargv);
          exit(fn(sh, argc, xargv));
        }
        execvp(xargv[0], xargv);
        if (errno == ENOENT)
          fprintf(stderr, "bang-lish: %s: command not found\n", xargv[0]);
        else
          perror(xargv[0]);
      }
      exit(127);
    }

    /* Parent */
    if (pgid == 0)
      pgid = pid;
    setpgid(pid, pgid);
    pids[i] = pid;
    expand_free_words(xargv);
  }

  if (sh->interactive)
    give_terminal_to(pgid);

  for (int i = 0; i < n - 1; i++) {
    close(pipes[i][0]);
    close(pipes[i][1]);
  }

  int last_status = 0;
  for (int i = 0; i < n; i++) {
    int wstatus;
    waitpid(pids[i], &wstatus, WUNTRACED);
    if (i == n - 1) {
      if (WIFEXITED(wstatus))
        last_status = WEXITSTATUS(wstatus);
      else if (WIFSIGNALED(wstatus))
        last_status = 128 + WTERMSIG(wstatus);
      else if (WIFSTOPPED(wstatus))
        last_status = 128 + WSTOPSIG(wstatus);
    }
  }

  if (sh->interactive)
    give_terminal_to(getpgrp());

  mem_free(pipes);
  mem_free(pids);
  return last_status;
}
