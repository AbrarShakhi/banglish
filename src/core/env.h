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
void env_set(EnvTable *et, const char *name, const char *value);
const char *env_get(EnvTable *et, const char *name);
void env_unset(EnvTable *et, const char *name);
void env_export(EnvTable *et, const char *name);
int env_is_exported(EnvTable *et, const char *name);
void env_load_environ(EnvTable *et);
