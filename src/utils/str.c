#include "str.h"

#include <ctype.h>
#include <stdlib.h>
#include <string.h>

#include "memory.h"

char *str_concat(const char *a, const char *b) {
  if (!a)
    a = "";
  if (!b)
    b = "";
  size_t la = strlen(a);
  size_t lb = strlen(b);
  char *r = mem_alloc(la + lb + 1);
  memcpy(r, a, la);
  memcpy(r + la, b, lb + 1);
  return r;
}

char *str_concat3(const char *a, const char *b, const char *c) {
  char *ab = str_concat(a, b);
  char *abc = str_concat(ab, c);
  mem_free(ab);
  return abc;
}

int str_startswith(const char *s, const char *prefix) {
  return strncmp(s, prefix, strlen(prefix)) == 0;
}

int str_endswith(const char *s, const char *suffix) {
  size_t ls = strlen(s);
  size_t lp = strlen(suffix);
  return lp <= ls && strcmp(s + ls - lp, suffix) == 0;
}

char *str_trim(const char *s) {
  while (isspace((unsigned char)*s))
    s++;
  const char *end = s + strlen(s);
  while (end > s && isspace((unsigned char)*(end - 1)))
    end--;
  size_t len = (size_t)(end - s);
  char *r = mem_alloc(len + 1);
  memcpy(r, s, len);
  r[len] = '\0';
  return r;
}

/* Split on ':' (for $PATH). Returns NULL-terminated array; caller frees with
   str_array_free(). */
char **str_split_colon(const char *s, int *out_count) {
  int count = 1;
  for (const char *p = s; *p; p++)
    if (*p == ':')
      count++;

  char **parts = mem_alloc((size_t)(count + 1) * sizeof(char *));
  int idx = 0;
  const char *start = s;
  for (const char *p = s;; p++) {
    if (*p == ':' || *p == '\0') {
      parts[idx++] = mem_strndup(start, (size_t)(p - start));
      if (*p == '\0')
        break;
      start = p + 1;
    }
  }
  parts[idx] = NULL;
  if (out_count)
    *out_count = idx;
  return parts;
}

void str_array_free(char **arr) {
  if (!arr)
    return;
  for (int i = 0; arr[i]; i++)
    mem_free(arr[i]);
  mem_free(arr);
}

int str_array_len(char **arr) {
  if (!arr)
    return 0;
  int n = 0;
  while (arr[n])
    n++;
  return n;
}

char *str_join(char **arr, const char *sep) {
  if (!arr || !arr[0])
    return mem_strdup("");
  size_t seplen = sep ? strlen(sep) : 0;
  size_t total = 0;
  for (int i = 0; arr[i]; i++)
    total += strlen(arr[i]) + (i > 0 ? seplen : 0);
  char *r = mem_alloc(total + 1);
  char *pos = r;
  for (int i = 0; arr[i]; i++) {
    if (i > 0 && sep) {
      memcpy(pos, sep, seplen);
      pos += seplen;
    }
    size_t l = strlen(arr[i]);
    memcpy(pos, arr[i], l);
    pos += l;
  }
  *pos = '\0';
  return r;
}
