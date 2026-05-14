#pragma once

typedef struct {
  char *name;
  char *value;
  int exported;
} VarEntry;

typedef struct {
  VarEntry *entries;
  int count;
  int cap;
} EnvTable;

EnvTable *env_create(void);
void env_destroy(EnvTable *et);
