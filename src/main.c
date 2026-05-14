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

  int final = sh->last_status;
  shell_destroy(sh);
  return final;
}
