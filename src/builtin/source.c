#include "source.h"

#include <stdio.h>

#include "exec/eval.h"

int builtin_source(ShellState *sh, int argc, char **argv) {
  if (argc < 2) {
    fprintf(stderr, "source: usage: source FILE [args...]\n");
    return 1;
  }
  /* Pass remaining args as positional parameters */
  int saved_argc = sh->script_argc;
  char **saved_argv = sh->script_argv;

  sh->script_argc = argc - 1;
  sh->script_argv = argv + 1;

  int ret = eval_file(sh, argv[1]);

  sh->script_argc = saved_argc;
  sh->script_argv = saved_argv;
  return ret;
}
