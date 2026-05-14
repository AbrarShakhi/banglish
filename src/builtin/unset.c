#include "unset.h"

#include <stdio.h>

#include "core/env.h"

int builtin_unset(ShellState *sh, int argc, char **argv) {
  if (argc < 2) {
    fprintf(stderr, "unset: usage: unset NAME...\n");
    return 1;
  }
  for (int i = 1; i < argc; i++)
    env_unset(sh->env, argv[i]);
  return 0;
}
