#pragma once
#include "core/shell.h"

typedef struct AliasEntry {
  char *name;
  char *value;
  struct AliasEntry *next;
} AliasEntry;

typedef struct AliasTable {
  AliasEntry *head;
} AliasTable;


int builtin_alias(ShellState *sh, int argc, char **argv);
int builtin_unalias(ShellState *sh, int argc, char **argv);
