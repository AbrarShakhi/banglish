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

AliasTable *aliastable_create(void);
void aliastable_destroy(AliasTable *at);
void aliastable_set(AliasTable *at, const char *name, const char *value);
const char *aliastable_get(AliasTable *at, const char *name);
void aliastable_unset(AliasTable *at, const char *name);

int builtin_alias(ShellState *sh, int argc, char **argv);
int builtin_unalias(ShellState *sh, int argc, char **argv);
