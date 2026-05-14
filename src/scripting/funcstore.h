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
