#include "help.h"

#include <stdio.h>

int builtin_help(ShellState *sh, int argc, char **argv) {
  (void)sh;
  (void)argc;
  (void)argv;
  puts("Bang-lish — Bangla+English production shell\n"
       "\nBuiltin commands:\n"
       "  cd [DIR]          Change directory (default: $HOME)\n"
       "  echo [-n] [ARGS]  Print arguments\n"
       "  exit [CODE]       Exit the shell\n"
       "  pwd               Print working directory\n"
       "  jobs              List background jobs\n"
       "  fg [%N]           Bring job N to foreground\n"
       "  bg [%N]           Resume stopped job N in background\n"
       "  source FILE       Execute FILE in current shell\n"
       "  unset NAME...     Remove shell variables\n"
       "  type NAME...      Show type of each NAME\n"
       "  which CMD...      Show path of CMD\n"
       "  alias [name=val]  Define or list aliases\n"
       "  unalias NAME...   Remove aliases\n"
       "  history [N]       Show last N history entries\n"
       "  help              Show this help\n"
       "\nBang-lish scripting:\n"
       "  let name = value  Assign shell variable\n"
       "  export name       Export variable to environment\n"
       "  def name(p1 p2) { body }  Define function\n"
       "  if CMD { ... } elif CMD { ... } else { ... }\n"
       "  while CMD { ... }\n"
       "  for var in words { ... }\n");
  return 0;
}
