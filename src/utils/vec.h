#pragma once

typedef struct {
  void **data;
  int count;
  int cap;
} Vec;

Vec *vec_create(void);
void vec_push(Vec *v, void *item);
void *vec_get(Vec *v, int idx);
int vec_count(Vec *v);
void vec_destroy(Vec *v);
void vec_destroy_items(Vec *v, void (*free_fn)(void *));
void **vec_to_null_terminated(Vec *v);
