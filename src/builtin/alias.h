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
