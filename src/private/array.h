/*
 * Copyright 2017 Patrick O. Perry.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifndef UTF8LITE_ARRAY_H
#define UTF8LITE_ARRAY_H

/**
 * \file array.h
 *
 * Dynamic array, growing to accommodate more elements.
 */

#include <stddef.h>

/**
 * Grow an array to accommodate more elements, possibly re-allocating.
 *
 * \param baseptr pointer to pointer to first element
 * \param sizeptr pointer to the capacity (in elements) of the array
 * \param width size of each element
 * \param count number of occupied elements
 * \param nadd number of elements to append after the `count` occupied
 *        elements
 *
 * \returns 0 on success
 */
int utf8lite_array_grow(void **baseptr, int *sizeptr, size_t width, int count,
			int nadd);

/**
 * Determine the capacity for an array that needs to grow.
 *
 * \param sizeptr pointer to the capacity (in elements) of the array
 * \param width size of each element
 * \param count number of occupied elements
 * \param nadd number of elements to append after the `count` occupied
 *        elements
 *
 * \returns 0 on success, `UTF8LITE_ERROR_OVERFLOW` on overflow
 */
int utf8lite_array_size_add(int *sizeptr, size_t width, int count, int nadd);

/**
 * Grow an big array to accommodate more elements, possibly re-allocating.
 *
 * \param baseptr pointer to pointer to first element
 * \param sizeptr pointer to the capacity (in elements) of the array
 * \param width size of each element
 * \param count number of occupied elements
 * \param nadd number of elements to append after the `count` occupied
 *        elements
 *
 * \returns 0 on success
 */
int utf8lite_bigarray_grow(void **baseptr, size_t *sizeptr, size_t width,
			   size_t count, size_t nadd);

/**
 * Determine the capacity for an array that needs to grow.
 *
 * \param sizeptr pointer to the capacity (in elements) of the array
 * \param width size of each element
 * \param count number of occupied elements
 * \param nadd number of elements to append after the `count` occupied
 *        elements
 *
 * \returns 0 on success, `UTF8LITE_ERROR_OVERFLOW` on overflow
 */
int utf8lite_bigarray_size_add(size_t *sizeptr, size_t width, size_t count,
			       size_t nadd);

#endif /* UTF8LITE_ARRAY_H */
