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

#ifndef RUTF8_H
#define RUTF8_H

#include <stddef.h>
#include <stdint.h>
#include <Rdefines.h>
#include "utf8lite/src/utf8lite.h"


#define CHECK_EVERY 1000
#define CHECK_INTERRUPT(i) \
	do { \
		if (((i) + 1) % CHECK_EVERY == 0) { \
			R_CheckUserInterrupt(); \
		} \
	} while (0)

#define TRY(x) \
	do { \
		if ((err = (x))) { \
			goto out; \
		} \
	} while (0)

#define TRY_ALLOC(x) \
	do { \
		if ((err = (x) ? 0 : UTF8LITE_ERROR_NOMEM)) { \
			goto out; \
		} \
	} while (0)

#define CHECK_ERROR_FORMAT_SEP(err, sep, fmt, ...) \
	do { \
		switch (err) { \
		case 0: \
			break; \
		case UTF8LITE_ERROR_INVAL: \
			Rf_error(fmt sep "invalid input", __VA_ARGS__); \
			break; \
		case UTF8LITE_ERROR_NOMEM: \
			Rf_error(fmt sep "memory allocation failure", \
				 __VA_ARGS__); \
			break; \
		case UTF8LITE_ERROR_OS: \
			Rf_error(fmt sep "operating system error", \
				 __VA_ARGS__); \
			break; \
		case UTF8LITE_ERROR_OVERFLOW: \
			Rf_error(fmt sep "overflow error", __VA_ARGS__); \
			break; \
		case UTF8LITE_ERROR_DOMAIN: \
			Rf_error(fmt sep "domain error", __VA_ARGS__); \
			break; \
		case UTF8LITE_ERROR_RANGE: \
			Rf_error(fmt sep "range error", __VA_ARGS__); \
			break; \
		case UTF8LITE_ERROR_INTERNAL: \
			Rf_error(fmt sep "internal error", __VA_ARGS__); \
			break; \
		default: \
			Rf_error(fmt sep "unknown error", __VA_ARGS__); \
			break; \
		} \
	} while (0)

#define CHECK_ERROR_FORMAT(err, fmt, ...) \
	CHECK_ERROR_FORMAT_SEP(err, fmt, ": ", __VA_ARGS__)

#define CHECK_ERROR_MESSAGE(err, msg) \
	CHECK_ERROR_FORMAT(err, "%s", msg)

#define CHECK_ERROR(err) \
	CHECK_ERROR_FORMAT_SEP(err, "", "%s", "")

/* context */
SEXP alloc_context(size_t size, void (*destroy_func)(void *));
void free_context(SEXP x);
void *as_context(SEXP x);
int is_context(SEXP x);

/* printing */
SEXP print_table(SEXP x, SEXP print_gap, SEXP right, SEXP max, SEXP width,
		 SEXP is_stdout);

/* utf8 */
SEXP utf8_coerce(SEXP x);
SEXP utf8_encode(SEXP x, SEXP display, SEXP utf8);
SEXP utf8_format(SEXP x, SEXP trim, SEXP chars, SEXP justify, SEXP width,
		 SEXP na_encode, SEXP quote, SEXP na_print, SEXP utf8);
SEXP utf8_normalize(SEXP x, SEXP map_case, SEXP map_compat, SEXP map_quote,
		    SEXP remove_ignorable);
SEXP utf8_valid(SEXP x);
SEXP utf8_width(SEXP x, SEXP quote, SEXP utf8);

/* internal utility functions */

int array_size_add(int *sizeptr, size_t width, int count, int nadd);
int charwidth(uint32_t code);
int charsxp_width(SEXP charsxp, int quote, int utf8);
int encodes_utf8(cetype_t ce);
const char *translate_utf8(SEXP x);

#endif /* RUTF8_H */
