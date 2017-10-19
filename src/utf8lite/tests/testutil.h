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

#ifndef TESTUTIL_H
#define TESTUTIL_H

#include <check.h>
#include <stddef.h>
#include <stdint.h>

struct utf8lite_text;

/**
 * This macro is broken on the old version of check (0.9.8) that Travis CI
 * uses, so we re-define it.
 */
#ifdef ck_assert_int_eq
#  undef ck_assert_int_eq
#endif
#define ck_assert_int_eq(X, Y) do { \
	intmax_t _ck_x = (X); \
	intmax_t _ck_y = (Y); \
	ck_assert_msg(_ck_x == _ck_y, \
			"Assertion '%s' failed: %s == %jd, %s == %jd", \
			#X " == " #Y, #X, _ck_x, #Y, _ck_y); \
} while (0)


/**
 * This macro doesn't exist on check version 0.9.8.
 */
#ifdef ck_assert_uint_eq
#  undef ck_assert_uint_eq
#endif
#define ck_assert_uint_eq(X, Y) do { \
	uintmax_t _ck_x = (X); \
	uintmax_t _ck_y = (Y); \
	ck_assert_msg(_ck_x == _ck_y, \
			"Assertion '%s' failed: %s == %ju, %s == %ju", \
			#X " == " #Y, #X, _ck_x, #Y, _ck_y); \
} while (0)


/**
 * Broken on check (0.9.8)
 */
#ifdef ck_assert_str_eq
#  undef ck_assert_str_eq
#endif
#define ck_assert_str_eq(X, Y) do { \
	const char* _ck_x = (X); \
	const char* _ck_y = (Y); \
	const char* _ck_x_s; \
	const char* _ck_y_s; \
	const char* _ck_x_q; \
	const char* _ck_y_q; \
	if (_ck_x != NULL) { \
		_ck_x_q = "\""; \
		_ck_x_s = _ck_x; \
	} else { \
		_ck_x_q = ""; \
		_ck_x_s = "(null)"; \
	} \
	if (_ck_y != NULL) { \
		_ck_y_q = "\""; \
		_ck_y_s = _ck_y; \
	} else { \
		_ck_y_q = ""; \
		_ck_y_s = "(null)"; \
	} \
	ck_assert_msg( \
		((_ck_x != NULL) && (_ck_y != NULL) \
		 && (0 == strcmp(_ck_y, _ck_x))), \
		 "Assertion '%s' failed: %s == %s%s%s, %s == %s%s%s", \
		#X" == "#Y, \
		#X, _ck_x_q, _ck_x_s, _ck_x_q, \
		#Y, _ck_y_q, _ck_y_s, _ck_y_q); \
} while (0)



#define assert_text_eq(X, Y) do { \
	const struct utf8lite_text * _ck_x = (X); \
	const struct utf8lite_text * _ck_y = (Y); \
	ck_assert_msg(utf8lite_text_equals(_ck_y, _ck_x), \
		"Assertion '%s == %s' failed: %s == \"%.*s\" (0x%zx)," \
		" %s==\"%.*s\" (0x%zx)", \
		#X, #Y, \
		#X, (int)UTF8LITE_TEXT_SIZE(_ck_x), _ck_x->ptr, _ck_x->attr, \
		#Y, (int)UTF8LITE_TEXT_SIZE(_ck_y), _ck_y->ptr, _ck_y->attr); \
} while (0)


#define assert_text_ne(X, Y) do { \
	const struct utf8lite_text * _ck_x = (X); \
	const struct utf8lite_text * _ck_y = (Y); \
	ck_assert_msg(!utf8lite_text_equals(_ck_y, _ck_x), \
		"Assertion '%s != %s' failed: %s == \"%s\" (0x%zx)," \
		" %s==\"%s\" (0x%zx)", \
		#X, #Y, \
		#X, (int)UTF8LITE_TEXT_SIZE(_ck_x), _ck_x->ptr, _ck_x->attr, \
		#Y, (int)UTF8LITE_TEXT_SIZE(_ck_y), _ck_y->ptr, _ck_y->attr); \
} while (0)


/**
 * Common test framework set up.
 */
void setup(void);

/**
 * Common test framework tear down.
 */ 
void teardown(void);

/**
 * Allocate memory.
 */
void *alloc(size_t size);

/**
 * Allocate a text object, interpreting JSON-style escape codes.
 */
struct utf8lite_text *JS(const char *str);

/**
 * Cast a raw string as a text object, ignoring escape codes.
 */
struct utf8lite_text *S(const char *str);

#endif /* TESTUTIL_H */
