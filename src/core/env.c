#include "env.h"

#include <stdlib.h>
#include <string.h>

#include "utils/memory.h"

EnvTable *env_create(void) {
  EnvTable *et = mem_alloc(sizeof(EnvTable));
  et->entries = NULL;
  et->count = 0;
  et->cap = 0;
  return et;
}

void env_destroy(EnvTable *et) {
  for (int i = 0; i < et->count; i++) {
    mem_free(et->entries[i].name);
    mem_free(et->entries[i].value);
  }
  mem_free(et->entries);
  mem_free(et);
}

static int find_entry(const EnvTable *et, const char *name) {
  for (int i = 0; i < et->count; i++)
    if (strcmp(et->entries[i].name, name) == 0)
      return i;
  return -1;
}

static void grow_if_needed(EnvTable *et) {
  if (et->count < et->cap)
    return;
  int new_cap = et->cap == 0 ? 16 : et->cap * 2;
  et->entries = mem_realloc(et->entries, (size_t)new_cap * sizeof(VarEntry));
  et->cap = new_cap;
}

void env_set(EnvTable *et, const char *name, const char *value) {
  int idx = find_entry(et, name);
  if (idx >= 0) {
    mem_free(et->entries[idx].value);
    et->entries[idx].value = mem_strdup(value);
    if (et->entries[idx].exported)
      setenv(name, value, 1);
    return;
  }
  grow_if_needed(et);
  et->entries[et->count].name = mem_strdup(name);
  et->entries[et->count].value = mem_strdup(value);
  et->entries[et->count].exported = 0;
  et->count++;
}

/* Checks local table first, then falls through to getenv(). */
const char *env_get(EnvTable *et, const char *name) {
  int idx = find_entry(et, name);
  if (idx >= 0)
    return et->entries[idx].value;
  return getenv(name);
}

void env_unset(EnvTable *et, const char *name) {
  int idx = find_entry(et, name);
  if (idx < 0)
    return;
  mem_free(et->entries[idx].name);
  mem_free(et->entries[idx].value);
  /* Shift remaining entries down */
  for (int i = idx; i < et->count - 1; i++)
    et->entries[i] = et->entries[i + 1];
  et->count--;
  unsetenv(name);
}

void env_export(EnvTable *et, const char *name) {
  int idx = find_entry(et, name);
  if (idx >= 0) {
    et->entries[idx].exported = 1;
    setenv(name, et->entries[idx].value, 1);
    return;
  }
  /* Variable not in local table; if it exists in environ, import it */
  const char *val = getenv(name);
  if (val) {
    env_set(et, name, val);
    idx = find_entry(et, name);
    et->entries[idx].exported = 1;
    /* Already in environ via getenv, no need to call setenv */
  }
}

int env_is_exported(EnvTable *et, const char *name) {
  int idx = find_entry(et, name);
  return idx >= 0 && et->entries[idx].exported;
}

/* Placeholder: we rely on getenv() fallthrough, so no bulk import needed. */
void env_load_environ(EnvTable *et) { (void)et; }
