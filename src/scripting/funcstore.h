#pragma once
#include "parser/ast.h"

typedef struct FuncEntry {
  char *name;    /* heap-owned */
  char **params; /* NULL-terminated; heap-owned strings */
  AstNode *body; /* owned by this entry */
  struct FuncEntry *next;
} FuncEntry;

typedef struct FuncTable {
  FuncEntry *head;
} FuncTable;

FuncTable *functable_create(void);
void functable_destroy(FuncTable *ft);
void functable_define(FuncTable *ft, const char *name, char **params,
                      AstNode *body);
FuncEntry *functable_find(FuncTable *ft, const char *name);
