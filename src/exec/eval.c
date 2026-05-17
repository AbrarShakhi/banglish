#include "eval.h"

#include <stdio.h>
#include <sys/wait.h>
#include <unistd.h>

#include "utils/memory.h"

int eval_string(ShellState *sh, const char *src) {
  if (!src || !*src)
    return 0;

  return -99;
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

  char *buf = mem_alloc((size_t)fsize);
  fclose(f);

  int status = eval_string(sh, buf);
  mem_free(buf);
  return status;
}
