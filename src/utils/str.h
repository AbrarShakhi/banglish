#pragma once
#include <stddef.h>

char  *str_concat(const char *a, const char *b);
char  *str_concat3(const char *a, const char *b, const char *c);
int    str_startswith(const char *s, const char *prefix);
int    str_endswith(const char *s, const char *suffix);
char  *str_trim(const char *s);
char **str_split_colon(const char *s, int *out_count);
void   str_array_free(char **arr);
int    str_array_len(char **arr);
char  *str_join(char **arr, const char *sep);
