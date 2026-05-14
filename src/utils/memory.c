#include "memory.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

void *mem_alloc(size_t size) {
  void *ptr = malloc(size);
  if (!ptr) {
    perror("bang-lish: malloc");
    exit(1);
  }
  return ptr;
}

void *mem_realloc(void *ptr, size_t size) {
  void *np = realloc(ptr, size);
  if (!np) {
    perror("bang-lish: realloc");
    exit(1);
  }
  return np;
}

void mem_free(void *ptr) { free(ptr); }

char *mem_strdup(const char *s) {
  if (!s)
    return NULL;
  char *copy = strdup(s);
  if (!copy) {
    perror("bang-lish: strdup");
    exit(1);
  }
  return copy;
}

char *mem_strndup(const char *s, size_t n) {
  if (!s)
    return NULL;
  char *copy = strndup(s, n);
  if (!copy) {
    perror("bang-lish: strndup");
    exit(1);
  }
  return copy;
}
