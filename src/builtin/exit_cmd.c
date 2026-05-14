#include "exit_cmd.h"

#include <stdio.h>
#include <stdlib.h>

#include "prompt/prompt.h"

int builtin_exit(ShellState *sh, int argc, char **argv) {
  int code = sh->last_status;
  if (argc >= 2) {
    char *end;
    long val = strtol(argv[1], &end, 10);
    if (*end == '\0')
      code = (int)(val & 0xFF);
    else
      fprintf(stderr, "exit: %s: numeric argument required\n", argv[1]);
  }
  prompt_teardown(sh);
  shell_destroy(sh);
  exit(code);
  return 0; /* unreachable */
}
