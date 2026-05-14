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

/* Checks local table first, then falls through to getenv(). */
const char *env_get(EnvTable *et, const char *name) {
  int idx = find_entry(et, name);
  if (idx >= 0)
    return et->entries[idx].value;
  return getenv(name);
}
