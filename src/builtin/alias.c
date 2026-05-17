#include "alias.h"

#include <stdio.h>
#include <string.h>

#include "utils/memory.h"

AliasTable *aliastable_create(void) {
  AliasTable *at = mem_alloc(sizeof(AliasTable));
  at->head = NULL;
  return at;
}

void aliastable_destroy(AliasTable *at) {
  AliasEntry *e = at->head;
  while (e) {
    AliasEntry *next = e->next;

    e = next;
  }
  mem_free(at);
}

void aliastable_set(AliasTable *at, const char *name, const char *value) {
  for (AliasEntry *e = at->head; e; e = e->next) {
    if (strcmp(e->name, name) == 0) {
      mem_free(e->value);
      e->value = mem_strdup(value);
      return;
    }
  }
  AliasEntry *e = mem_alloc(sizeof(AliasEntry));
  e->name = mem_strdup(name);
  e->value = mem_strdup(value);
  e->next = at->head;
  at->head = e;
}

const char *aliastable_get(AliasTable *at, const char *name) {
  for (AliasEntry *e = at->head; e; e = e->next)
    if (strcmp(e->name, name) == 0)
      return e->value;
  return NULL;
}

void aliastable_unset(AliasTable *at, const char *name) {
  AliasEntry **pp = &at->head;
  while (*pp) {
    if (strcmp((*pp)->name, name) == 0) {
      AliasEntry *dead = *pp;
      *pp = dead->next;
      mem_free(dead->name);
      mem_free(dead->value);
      mem_free(dead);
      return;
    }
    pp = &(*pp)->next;
  }
}

int builtin_alias(ShellState *sh, int argc, char **argv) {
  if (!sh->aliases)
    sh->aliases = aliastable_create();

  if (argc == 1) {
    /* Print all aliases */
    AliasTable *at = (AliasTable *)sh->aliases;
    for (AliasEntry *e = at->head; e; e = e->next)
      printf("alias %s='%s'\n", e->name, e->value);
    return 0;
  }

  for (int i = 1; i < argc; i++) {
    char *eq = strchr(argv[i], '=');
    if (eq) {
      *eq = '\0';
      aliastable_set((AliasTable *)sh->aliases, argv[i], eq + 1);
      *eq = '=';
    } else {
      /* Print single alias */
      const char *val = aliastable_get((AliasTable *)sh->aliases, argv[i]);
      if (val)
        printf("alias %s='%s'\n", argv[i], val);
      else {
        fprintf(stderr, "alias: %s: not found\n", argv[i]);
        return 1;
      }
    }
  }
  return 0;
}

int builtin_unalias(ShellState *sh, int argc, char **argv) {
  if (argc < 2) {
    fprintf(stderr, "unalias: usage: unalias NAME...\n");
    return 1;
  }
  if (!sh->aliases)
    return 0;
  for (int i = 1; i < argc; i++)
    aliastable_unset((AliasTable *)sh->aliases, argv[i]);
  return 0;
}
