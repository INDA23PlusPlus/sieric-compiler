/*
 * Copyright 2023 Emma Ericsson
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice,
 * this list of conditions and the following disclaimer.
 *
 * 2. Redistributions in binary form must reproduce the above copyright notice,
 * this list of conditions and the following disclaimer in the documentation
 * and/or other materials provided with the distribution.
 *
 * 3. Neither the name of the copyright holder nor the names of its contributors
 * may be used to endorse or promote products derived from this software without
 * specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS “AS IS”
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

/**
 * @file
 * @copydoc utils/vector.h
 */
#include <utils/vector.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

static inline int vec_compar_eq(vec_item_t, vec_item_t);
static inline void vec_print_str(FILE *, vec_item_t);
static inline void vec_print_ptr(FILE *, vec_item_t);

inline vec_t *vec_init(vec_t *vec, size_t n) {
    vec->items = malloc(n*sizeof(vec_item_t));
    vec->free_fn = NULL;
    vec->sz = 0;
    vec->capacity = n;

    return vec;
}

inline vec_t *vec_new(size_t n) {
    vec_t *vec = malloc(sizeof(vec_t));
    return vec_init(vec, n);
}

inline vec_t *vec_init_free(vec_t *vec, size_t n, vec_free_t fn) {
    vec_init(vec, n);
    vec->free_fn = fn;
    return vec;
}

inline vec_t *vec_new_free(size_t n, vec_free_t fn) {
    vec_t *vec = malloc(sizeof(vec_t));
    return vec_init_free(vec, n, fn);
}

inline vec_t *vec_init_copy(vec_t *dst, vec_t *src) {
    dst->capacity = dst->sz = src->sz;
    dst->free_fn = src->free_fn;
    dst->items = malloc(sizeof(vec_item_t)*dst->capacity);
    memcpy(dst->items, src->items, sizeof(vec_item_t)*dst->sz);
    return dst;
}

inline vec_t *vec_new_copy(vec_t *src) {
    vec_t *dst = malloc(sizeof(vec_t));
    return vec_init_copy(dst, src);
}

inline void vec_free(vec_t *vec) {
    vec_destroy(vec);
    free(vec);
}

inline void vec_destroy(vec_t *vec) {
    if(vec->free_fn)
        for(size_t i = 0; i < vec->sz; ++i)
            vec->free_fn(vec->items[i]);
    free(vec->items);
}

inline vec_item_t vec_push(vec_t *vec, vec_item_t item) {
    if(++vec->sz > vec->capacity)
        vec->items = realloc(vec->items, sizeof(vec_item_t)
                             * (vec->capacity = (vec->capacity*3 + 1)>>1));
    return vec->items[vec->sz-1] = item;
}

inline vec_item_t vec_pop(vec_t *vec) {
    return vec->sz == 0 ? NULL : vec->items[--vec->sz];
}

inline void vec_pop_free(vec_t *vec) {
    if(vec->sz == 0) return;
    vec_item_t item = vec->items[--vec->sz];
    if(vec->free_fn) vec->free_fn(item);
}

inline vec_item_t vec_insert(vec_t *vec, size_t idx, vec_item_t item) {
    if(++vec->sz > vec->capacity)
        vec->items = realloc(vec->items, sizeof(vec_item_t)
                             * (vec->capacity = (vec->capacity*3 + 1)>>1));
    memmove(vec->items + idx + 1, vec->items + idx,
            sizeof(vec_item_t) * (vec->sz-1-idx));
    return vec->items[idx] = item;
}

inline vec_item_t vec_delete(vec_t *vec, size_t idx) {
    vec_item_t out = vec->items[idx];
    memmove(vec->items + idx, vec->items + idx + 1,
            sizeof(vec_item_t) * (--vec->sz - idx));
    return out;
}

inline void vec_delete_free(vec_t *vec, size_t idx) {
    vec_item_t item = vec_delete(vec, idx);
    if(vec->free_fn) vec->free_fn(item);
}

inline vec_item_t vec_set(vec_t *vec, size_t idx, vec_item_t item) {
    vec_item_t prev = vec->items[idx];
    vec->items[idx] = item;
    return prev;
}

inline void vec_set_free(vec_t *vec, size_t idx, vec_item_t item) {
    if(vec->free_fn) vec->free_fn(vec->items[idx]);
    vec->items[idx] = item;
}

inline vec_item_t vec_get(vec_t *vec, size_t idx) {
    return vec->items[idx];
}

inline size_t vec_reserve(vec_t *vec, size_t n) {
    while(vec->capacity < n) vec->capacity = (vec->capacity*3 + 1)>>1;
    return vec->capacity;
}

inline size_t vec_reserve_add(vec_t *vec, size_t n) {
    return vec_reserve(vec, n + vec->capacity);
}

inline size_t vec_reserve_exact(vec_t *vec, size_t n) {
    if(vec->capacity < n) vec->capacity = n;
    return vec->capacity;
}

inline size_t vec_reserve_add_exact(vec_t *vec, size_t n) {
    return vec->capacity += n;
}

inline size_t vec_shrink(vec_t *vec, size_t n) {
    if(vec->free_fn && vec->sz > n)
        for(size_t i = n; i < vec->sz; ++i)
            vec->free_fn(vec->items[i]);
    if(vec->capacity > n) vec->items = realloc(vec->items, vec->capacity = n);
    return vec->capacity;
}

inline vec_t *vec_merge(vec_t *dst, vec_t *src) {
    vec_reserve_exact(dst, dst->sz + src->sz);
    memcpy(dst->items + dst->sz, src->items, sizeof(vec_item_t)*src->sz);
    dst->sz += src->sz;
    return dst;
}

inline vec_t *vec_extend(vec_t *dst, vec_item_t *arr, size_t sz) {
    vec_reserve_add(dst, sz);
    memcpy(dst->items + dst->sz, arr, sizeof(vec_item_t)*sz);
    dst->sz += sz;
    return dst;
}

inline vec_t *vec_extend_exact(vec_t *dst, vec_item_t *arr, size_t sz) {
    vec_reserve_add_exact(dst, sz);
    memcpy(dst->items + dst->sz, arr, sizeof(vec_item_t)*sz);
    dst->sz += sz;
    return dst;
}

inline ssize_t vec_index_of(vec_t *vec, vec_item_t item, vec_compar_t compar) {
    for(size_t i = 0; i < vec->sz; ++i)
        if(!compar(vec->items[i], item))
            return i;
    return -1;
}

static inline int vec_compar_eq(vec_item_t l, vec_item_t r) {
    return (intptr_t)l - (intptr_t)r;
}

inline ssize_t vec_index_of_eq(vec_t *vec, vec_item_t item) {
    return vec_index_of(vec, item, vec_compar_eq);
}

static inline void vec_print_str(FILE *f, vec_item_t item) {
    fprintf(f, "%s", (char *)item);
}

static inline void vec_print_ptr(FILE *f, vec_item_t item) {
    fprintf(f, "%p", (void *)item);
}

inline void vec_debug_dump(FILE *f, vec_t *vec,
                           void (*print)(FILE *, vec_item_t)) {
    fputc('{', f);
    for(size_t i = 0; i < vec->sz; ++i) print(f, vec->items[i]), fputc(',', f);
    fputc('}', f);
}

inline void vec_debug_dump_str(FILE *f, vec_t *vec) {
    vec_debug_dump(f, vec, vec_print_str);
}

inline void vec_debug_dump_ptr(FILE *f, vec_t *vec) {
    vec_debug_dump(f, vec, vec_print_ptr);
}
