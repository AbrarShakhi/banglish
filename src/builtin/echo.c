#include "echo.h"

#include <stdio.h>
#include <string.h>

int builtin_echo(ShellState *sh, int argc, char **argv) {
  (void)sh;
  int no_newline = 0;
  int start = 1;

  /* Handle -n flag */
  if (argc >= 2 && strcmp(argv[1], "-n") == 0) {
    no_newline = 1;
    start = 2;
  }

  for (int i = start; i < argc; i++) {
    if (i > start)
      putchar(' ');
    /* Interpret \n, \t, \\ escape sequences */
    const char *s = argv[i];
    while (*s) {
      if (*s == '\\' && *(s + 1)) {
        s++;
        switch (*s) {
        case 'n':
          putchar('\n');
          break;
        case 't':
          putchar('\t');
          break;
        case 'r':
          putchar('\r');
          break;
        case '\\':
          putchar('\\');
          break;
        case 'e':
          putchar('\033');
          break;
        default:
          putchar('\\');
          putchar(*s);
          break;
        }
      } else {
        putchar(*s);
      }
      s++;
    }
  }

  if (!no_newline)
    putchar('\n');
  return 0;
}
