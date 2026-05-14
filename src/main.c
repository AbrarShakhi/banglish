#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "core/shell.h"
#include "core/signal.h"
#include "exec/eval.h"
#include "prompt/prompt.h"
#include "utils/memory.h"

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
    int status = -999; // TODO:Run the strinng here
    shell_destroy(sh);
    return status;
  }

  if (arg_idx < argc) {
    /* Script file mode */
    sh->interactive = 0;
    sh->script_argc = argc - arg_idx;
    sh->script_argv = argv + arg_idx;
    int status = -999; // TODO:Run the file here
    shell_destroy(sh);
    return status;
  }

  /* Interactive or piped stdin */
  sh->interactive = isatty(STDIN_FILENO);

  if (sh->interactive) {

    const char *home = getenv("HOME");
    if (home) {
      char rc[PATH_MAX];
      snprintf(rc, sizeof(rc), "%s/.banglishrc", home);
      if (access(rc, F_OK) == 0)
        eval_file(sh, rc);
    }
  }

  int final = sh->last_status;
  shell_destroy(sh);
  return final;
}
