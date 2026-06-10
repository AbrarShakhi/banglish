#include "funcstore.h"

#include <string.h>

#include "utils/memory.h"
#include "utils/str.h"

FuncTable *functable_create(void) {
  FuncTable *ft = mem_alloc(sizeof(FuncTable));
  ft->head = NULL;
  return ft;
}

void functable_destroy(FuncTable *ft) {
  FuncEntry *e = ft->head;
  while (e) {
    FuncEntry *next = e->next;
    mem_free(e->name);
    str_array_free(e->params);
    ast_free(e->body);
    mem_free(e);
    e = next;
  }
  mem_free(ft);
}

void functable_define(FuncTable *ft, const char *name, char **params,
                      AstNode *body) {
  /* Replace existing definition if present */
  for (FuncEntry *e = ft->head; e; e = e->next) {
    if (strcmp(e->name, name) == 0) {
      str_array_free(e->params);
      ast_free(e->body);
      /* Duplicate params */
      int n = str_array_len(params);
      e->params = mem_alloc((size_t)(n + 1) * sizeof(char *));
      for (int i = 0; i < n; i++)
        e->params[i] = mem_strdup(params[i]);
      e->params[n] = NULL;
      e->body = body;
      return;
    }
  }
  FuncEntry *e = mem_alloc(sizeof(FuncEntry));
  e->name = mem_strdup(name);
  int n = str_array_len(params);
  e->params = mem_alloc((size_t)(n + 1) * sizeof(char *));
  for (int i = 0; i < n; i++)
    e->params[i] = mem_strdup(params[i]);
  e->params[n] = NULL;
  e->body = body;
  e->next = ft->head;
  ft->head = e;
}

FuncEntry *functable_find(FuncTable *ft, const char *name) {
  for (FuncEntry *e = ft->head; e; e = e->next)
    if (strcmp(e->name, name) == 0)
      return e;
  return NULL;
}
