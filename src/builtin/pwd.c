#include "pwd.h"

#include <stdio.h>

#include "info/info.h"
#include "utils/memory.h"

int builtin_pwd(ShellState *sh, int argc, char **argv) {
  (void)sh;
  (void)argc;
  (void)argv;
  char *cwd = info_get_cwd();
  puts(cwd);
  mem_free(cwd);
  return 0;
}
