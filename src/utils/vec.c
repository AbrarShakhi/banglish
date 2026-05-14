#include "vec.h"

#include <string.h>

#include "memory.h"

Vec *vec_create(void) {
	Vec *v = mem_alloc(sizeof(Vec));
	v->data = NULL;
	v->count = 0;
	v->cap = 0;
	return v;
}

void vec_push(Vec *v, void *item) {
	if (v->count >= v->cap) {
		int new_cap = v->cap == 0 ? 8 : v->cap * 2;
		v->data = mem_realloc(v->data, (size_t)new_cap * sizeof(void *));
		v->cap = new_cap;
	}
	v->data[v->count++] = item;
}

void *vec_get(Vec *v, int idx) { return v->data[idx]; }

int vec_count(Vec *v) { return v->count; }

void vec_destroy(Vec *v) {
	mem_free(v->data);
	mem_free(v);
}

void vec_destroy_items(Vec *v, void (*free_fn)(void *)) {
	for (int i = 0; i < v->count; i++)
		free_fn(v->data[i]);
	vec_destroy(v);
}

/* Returns a fresh NULL-terminated array; caller frees the array (not items). */
void **vec_to_null_terminated(Vec *v) {
	void **arr = mem_alloc((size_t)(v->count + 1) * sizeof(void *));
	memcpy(arr, v->data, (size_t)v->count * sizeof(void *));
	arr[v->count] = NULL;
	return arr;
}
