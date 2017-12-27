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

#define RUTF8_STYLE_CLOSE "\033[0m"
#define RUTF8_STYLE_CLOSE_SIZE 4

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
			goto exit; \
		} \
	} while (0)

#define TRY_ALLOC(x) \
	do { \
		if ((err = (x) ? 0 : UTF8LITE_ERROR_NOMEM)) { \
			goto exit; \
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

/**
 * Justification type.
 */
enum rutf8_justify_type {
	RUTF8_JUSTIFY_NONE = 0,
	RUTF8_JUSTIFY_LEFT,
	RUTF8_JUSTIFY_CENTRE,
	RUTF8_JUSTIFY_RIGHT
};

/**
 * String type indicator.
 */
enum rutf8_string_type {
	RUTF8_STRING_NONE = 0,	/**< missing value */
	RUTF8_STRING_BYTES,	/**< unknown encoding, not valid UTF-8 */
	RUTF8_STRING_TEXT	/**< valid UTF-8 */
};

/**
 * Raw bytes
 */
struct rutf8_bytes {
	const uint8_t *ptr; 	/**< data pointer */
	size_t size;		/**< number of bytes */
};

/**
 * String data, either UTF-8, bytes, or NA
 */
struct rutf8_string {
	union {
		struct utf8lite_text text;	/**< utf8 data */
		struct rutf8_bytes bytes;	/**< raw bytes */
	} value;				/**< string value */
	enum rutf8_string_type type;		/**< type indicator */
};

void rutf8_string_init(struct rutf8_string *str, SEXP charsxp);
int rutf8_string_width(const struct rutf8_string *str, int flags);
int rutf8_string_lwidth(const struct rutf8_string *str, int flags,
			int limit, int ellipsis);
int rutf8_string_rwidth(const struct rutf8_string *str, int flags,
			int limit, int ellipsis);
void rutf8_string_render(struct utf8lite_render *r,
			 const struct rutf8_string *str,
			 int width, int quote, enum rutf8_justify_type justify);
SEXP rutf8_string_format(struct utf8lite_render *r,
			 const struct rutf8_string *str,
			 int trim, int chars, enum rutf8_justify_type justify,
			 int quote, const char *ellipsis, size_t nellipsis,
			 int wellipsis, int flags, int width_max);

int rutf8_text_width(const struct utf8lite_text *text, int flags);
int rutf8_text_lwidth(const struct utf8lite_text *text, int flags,
		      int limit, int ellipsis);
int rutf8_text_rwidth(const struct utf8lite_text *text, int flags,
		      int limit, int ellipsis);
void rutf8_text_render(struct utf8lite_render *r,
		       const struct utf8lite_text *text,
		       int width, int quote, enum rutf8_justify_type justify);
SEXP rutf8_text_format(struct utf8lite_render *r,
		       const struct utf8lite_text *text,
		       int trim, int chars, enum rutf8_justify_type justify,
		       int quote, const char *ellipsis, size_t nellipsis,
		       int wellipsis, int flags, int width_max);

int rutf8_bytes_width(const struct rutf8_bytes *bytes, int flags);
int rutf8_bytes_lwidth(const struct rutf8_bytes *bytes, int flags, int limit);
int rutf8_bytes_rwidth(const struct rutf8_bytes *bytes, int flags, int limit);
void rutf8_bytes_render(struct utf8lite_render *r,
			const struct rutf8_bytes *bytes,
			int width_min, int quote,
			enum rutf8_justify_type justify);
SEXP rutf8_bytes_format(struct utf8lite_render *r,
			const struct rutf8_bytes *bytes,
			int trim, int chars, enum rutf8_justify_type justify,
			int quote, int flags, int width_max);

/* context */
SEXP rutf8_alloc_context(size_t size, void (*destroy_func)(void *));
void rutf8_free_context(SEXP x);
void *rutf8_as_context(SEXP x);
int rutf8_is_context(SEXP x);

/* render object */
SEXP rutf8_alloc_render(int flags);
void rutf8_free_render(SEXP x);
struct utf8lite_render *rutf8_as_render(SEXP x);
int rutf8_is_render(SEXP x);

/* printing */
SEXP rutf8_render_table(SEXP x, SEXP width, SEXP quote, SEXP na_print,
			SEXP print_gap, SEXP right, SEXP max, SEXP names,
			SEXP rownames, SEXP escapes, SEXP display, SEXP style,
			SEXP utf8, SEXP linewidth);

/* utf8 */
SEXP rutf8_as_utf8(SEXP x);
SEXP rutf8_utf8_encode(SEXP x, SEXP width, SEXP quote, SEXP justify,
		       SEXP escapes, SEXP display, SEXP utf8);
SEXP rutf8_utf8_format(SEXP x, SEXP trim, SEXP chars, SEXP justify,
		       SEXP width, SEXP na_encode, SEXP quote,
		       SEXP na_print, SEXP ellipsis, SEXP wellipsis,
		       SEXP utf8);
SEXP rutf8_utf8_normalize(SEXP x, SEXP map_case, SEXP map_compat,
			  SEXP map_quote, SEXP remove_ignorable);
SEXP rutf8_utf8_valid(SEXP x);
SEXP rutf8_utf8_width(SEXP x, SEXP encode, SEXP quote, SEXP utf8);

/* utility functions */
int rutf8_as_justify(SEXP justify);
const char *rutf8_as_style(SEXP style);
int rutf8_encodes_utf8(cetype_t ce);
const char *rutf8_translate_utf8(SEXP x);

#endif /* RUTF8_H */
