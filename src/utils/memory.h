#pragma once
#include <stddef.h>

void  *mem_alloc(size_t size);
void  *mem_realloc(void *ptr, size_t size);
void   mem_free(void *ptr);
char  *mem_strdup(const char *s);
char  *mem_strndup(const char *s, size_t n);
