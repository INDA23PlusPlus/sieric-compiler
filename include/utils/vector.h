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
 * @author Emma Ericsson <emma@krlsg.se>
 *
 * @brief Implementation of a basic vector type
 */
#ifndef CBASE_UTILS_VECTOR_H_
#define CBASE_UTILS_VECTOR_H_

#include <sys/types.h>
#include <stddef.h>
#include <stdio.h>

/** Representation of a vector item */
typedef void *vec_item_t;
/** A free function for a vector item */
typedef void (*vec_free_t)(vec_item_t);
/** A comparison function for vector items, should work like `memcmp` */
typedef int (*vec_compar_t)(vec_item_t, vec_item_t);

/** A dynamically sized array */
typedef struct vec {
    /** Internal array of items */
    vec_item_t *items;
    /** Function used to free items */
    vec_free_t free_fn;
    /** The current amount of items in the vector */
    size_t sz;
    /** The capacity of the vector */
    size_t capacity;
} vec_t;

/**
 * @brief Initializes a vector.
 *
 * @param[out] vec   Vector
 * @param[in]  n     Capacity
 * @return Vector
 */
vec_t *vec_init(vec_t *vec, size_t n);

/**
 * @brief Allocates and initializes a vector on the heap.
 *
 * @param[in] n   Capacity
 * @return Vector
 */
vec_t *vec_new(size_t n);

/**
 * @brief Initializes a vector.
 *
 * @param[out] vec  Capacity
 * @param[in]  n    Capacity
 * @param[in]  fn   Freeing function for elements. `NULL` means no freeing.
 * @return Vector
 */
vec_t *vec_init_free(vec_t *vec, size_t n, vec_free_t fn);

/**
 * @brief Allocates and initialized a vector on the heap.
 *
 * @param[in] n   Capacity
 * @param[in] fn  Freeing function for elements. `NULL` means no freeing.
 * @return Vector
 */
vec_t *vec_new_free(size_t n, vec_free_t fn);

/**
 * @brief Initializes a vector as a copy of another vector.
 *
 * Copies a vector using `memcpy`. Will therefore not work correctly with
 * pointers as items.
 *
 * @param[in] dst   Vector to copy into
 * @param[in] src   Vector to copy
 * @return New vector copy
 */
vec_t *vec_init_copy(vec_t *dst, vec_t *src);

/**
 * @brief Creates a vector on the heap as a copy of another vector.
 *
 * Copies a vector using `memcpy`. Will therefore not work correctly with
 * pointers as items.
 *
 * @param[in] src   Vector to copy
 * @return New vector copy
 */
vec_t *vec_new_copy(vec_t *src);

/**
 * @brief Frees a vector allocated on the heap.
 *
 * Free a vector and free its items if a freeing function has been provided.
 *
 * @param[in] vec   Vector
 */
void vec_free(vec_t *vec);

/**
 * @brief Destroys a vector by freeing all its items. Does not free the vector.
 *
 * @param[in] vec   Vector
 */
void vec_destroy(vec_t *vec);

/**
 * @brief Pushes an item to the end of the vector.
 *
 * @param[in] vec   Vector
 * @param[in] item  Item to push
 * @return Item that was pushed
 */
vec_item_t vec_push(vec_t *vec, vec_item_t item);

/**
 * @brief Pops an item from the end of the vector. Returns `NULL` if empty.
 *
 * @param[in] vec   Vector
 * @return Popped item
 */
vec_item_t vec_pop(vec_t *vec);

/**
 * @brief Pops an item from the end of the vector and frees it.
 *
 * @param[in] vec   Vector
 */
void vec_pop_free(vec_t *vec);

/**
 * @brief Inserts an item at the specified index in a vector.
 *
 * @param[in] vec   Vector
 * @param[in] idx   Index
 * @param[in] item  Item to insert
 * @return Item that was inserted
 */
vec_item_t vec_insert(vec_t *vec, size_t idx, vec_item_t item);

/**
 * @brief Deletes an item at the specified index in the vector.
 *
 * Deletes an item at the specified index in the vector. All items following the
 * deleted item will be moved back to fill the hole.
 *
 * @param[in] vec   Vector
 * @param[in] idx   Index
 * @return Deleted item
 */
vec_item_t vec_delete(vec_t *vec, size_t idx);

/**
 * @brief Deletes an item at the specified index in the vector and frees it.
 *
 * Deletes an item at the specified index in the vector and frees it. All items
 * following the deleted item will be moved back to fill the hole.
 *
 * @param[in] vec   Vector
 * @param[in] idx   Index
 */
void vec_delete_free(vec_t *vec, size_t idx);

/**
 * @brief Sets an item in the vector and returns the old item.
 *
 * @param[in] vec   Vector
 * @param[in] idx   Index
 * @param[in] item  New value
 * @return Old item
 */
vec_item_t vec_set(vec_t *vec, size_t idx, vec_item_t item);

/**
 * @brief Sets an item in the vector and frees the old item.
 *
 * @param[in] vec   Vector
 * @param[in] idx   Index
 * @param[in] item  New value
 */
void vec_set_free(vec_t *vec, size_t idx, vec_item_t item);

/**
 * @brief Gets the item at the specified index in the vector.
 *
 * @param[in] vec   Vector
 * @param[in] idx   Index
 * @return Item
 */
vec_item_t vec_get(vec_t *vec, size_t idx);

/**
 * @brief Increases the capacity of the vector to at least `n`.
 *
 * Increases the capacity of the vector to at least `n`. Will increease as if
 * items were pushed until the size reached `n`.
 *
 * @param[in] vec   Vector
 * @param[in] n     New capacity
 * @return New capacity
 */
size_t vec_reserve(vec_t *vec, size_t n);

/**
 * @brief Increases the capacity of the vector with at least `n`.
 *
 * Increases the capacity of the vector with at least `n`. Will increease as if
 * items were pushed until the size reached `n`.
 *
 * @param[in] vec   Vector
 * @param[in] n     Capacity to add
 * @return New capacity
 */
size_t vec_reserve_add(vec_t *vec, size_t n);

/**
 * @brief Sets the capacity of the vector to `n`.
 *
 * Sets the capacity of the vector to `n`. It will not shrink the vector.
 *
 * @param[in] vec   Vector
 * @param[in] n     New capacity
 * @return New capacity
 */
size_t vec_reserve_exact(vec_t *vec, size_t n);

/**
 * @brief Adds `n` to the capacity of the vector.
 *
 * @param[in] vec   Vector
 * @param[in] n     Capacity to add
 * @return New capacity
 */
size_t vec_reserve_add_exact(vec_t *vec, size_t n);

/**
 * @brief Shrinks the vector to the specified size.
 *
 * Shrinks the vector to the specified size. Will reduce the capacity and free
 * the removed items.
 *
 * @param[in] vec   Vector
 * @param[in] n     New capacity
 * @return New capacity
 */
size_t vec_shrink(vec_t *vec, size_t n);

/**
 * @brief Merges two lists into one.
 *
 * Merges two lists into one. Will copy values from the source to the
 * destination using `memcpy`.
 *
 * @param[in] dst   Destination vector
 * @param[in] src   Source vector
 * @return Destination vector
 */
vec_t *vec_merge(vec_t *dst, vec_t *src);

/**
 * @brief Extends a vector with values from an array.
 *
 * @param[in] dst   Destination vector
 * @param[in] arr   Source array
 * @param[in] sz    Size of arr
 * @return Destination vector
 */
vec_t *vec_extend(vec_t *dst, vec_item_t *arr, size_t sz);

/**
 * @brief Extends a vector with values from an array using `vec_reserve_exact`.
 *
 * @param[in] dst   Destination vector
 * @param[in] arr   Source array
 * @param[in] sz    Size of arr
 * @return Destination vector
 */
vec_t *vec_extend_exact(vec_t *dst, vec_item_t *arr, size_t sz);

/**
 * @brief Get the index of the first item which matches the specified item.
 *
 * Get the index of the first item which matches the specified item according to
 * the specified comparison function. If no matching item is found it returns
 * `-1`.
 *
 * @param[in] vec     Vector
 * @param[in] item    Item to compare to
 * @param[in] compar  Comparison function
 * @return Index of the matching item, or `-1`
 */
ssize_t vec_index_of(vec_t *vec, vec_item_t item, vec_compar_t compar);

/**
 * @brief Get the index of the first item which is equal to the specified item.
 *
 * Get the index of the first item which is equal to the specified item.
 * Equality is checked based on the value of item, not what it points to. If no
 * matching item is found it returns `-1`.
 *
 * @param[in] vec   Vector
 * @param[in] item  Item to compare to
 * @return Index of the matching item, or `-1`
 */
ssize_t vec_index_of_eq(vec_t *vec, vec_item_t item);

/**
 * @brief Prints the contents of a vector to a file using the provided print
 * function.
 *
 * @param[in] f      Output file
 * @param[in] vec    Vector to print
 * @param[in] print  Function to call on each item. Should not print newline.
 */
void vec_debug_dump(FILE *f, vec_t *vec, void (*print)(FILE *, vec_item_t));

/**
 * @brief Prints the contents of a vector to a file assuming items are strings.
 *
 * @param[in] f      Output file
 * @param[in] vec    Vector to print
 */
void vec_debug_dump_str(FILE *f, vec_t *vec);

/**
 * @brief Prints the contents of a vector to a file assuming items are pointers.
 *
 * @param[in] f      Output file
 * @param[in] vec    Vector to print
 */
void vec_debug_dump_ptr(FILE *f, vec_t *vec);

#endif /* CBASE_UTILS_VECTOR_H_ */
