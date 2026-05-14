#include "history_cmd.h"

#include <stdio.h>
#include <stdlib.h>

#ifdef HAVE_READLINE
#include <readline/history.h>
#endif

int builtin_history(ShellState *sh, int argc, char **argv) {
  (void)sh;
#ifdef HAVE_READLINE
  int limit = -1;
  if (argc >= 2) {
    limit = atoi(argv[1]);
    if (limit <= 0)
      limit = -1;
  }

  HIST_ENTRY **list = history_list();
  if (!list)
    return 0;

  int count = 0;
  while (list[count])
    count++;

  int start = 0;
  if (limit > 0 && limit < count)
    start = count - limit;

  for (int i = start; i < count; i++)
    printf("%5d  %s\n", i + 1, list[i]->line);
#else
  (void)argc;
  (void)argv;
  fprintf(stderr, "history: readline not available\n");
#endif
  return 0;
}
