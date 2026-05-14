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
