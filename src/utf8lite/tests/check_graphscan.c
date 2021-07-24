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

#include <stdio.h>
#include <stdlib.h>
#include <check.h>
#include "../src/utf8lite.h"
#include "testutil.h"

#define GRAPH_BREAK_TEST "data/ucd/auxiliary/GraphemeBreakTest.txt"
struct utf8lite_graphscan scan;


void setup_scan(void)
{
	setup();
}


void teardown_scan(void)
{
	teardown();
}


void start(const struct utf8lite_text *text)
{
	utf8lite_graphscan_make(&scan, text);
}


const struct utf8lite_text *next(void)
{
	struct utf8lite_text *graph;
	if (!utf8lite_graphscan_advance(&scan)) {
		return NULL;
	}
	graph = alloc(sizeof(*graph));
	*graph = scan.current.text;
	return graph;
}


const struct utf8lite_text *prev(void)
{
	struct utf8lite_text *graph;
	if (!utf8lite_graphscan_retreat(&scan)) {
		return NULL;
	}
	graph = alloc(sizeof(*graph));
	*graph = scan.current.text;
	return graph;
}


START_TEST(test_empty)
{
	start(S(""));
	ck_assert(next() == NULL);
	ck_assert(next() == NULL);
	ck_assert(prev() == NULL);
	ck_assert(prev() == NULL);
}
END_TEST


START_TEST(test_single)
{
	start(S("x"));
	ck_assert(prev() == NULL);
	assert_text_eq(next(), S("x"));
	ck_assert(prev() == NULL);
	assert_text_eq(next(), S("x"));
	ck_assert(next() == NULL);
	ck_assert(next() == NULL);
	assert_text_eq(prev(), S("x"));
	ck_assert(prev() == NULL);
	ck_assert(prev() == NULL);
}
END_TEST


START_TEST(test_emoji_modifier)
{
	// This is an Extended_Pictographic followed by Extend
	start(JS("\\uD83D\\uDE0A\\uD83C\\uDFFB")); // U+1F60A U+1F3FB
	assert_text_eq(next(), JS("\\uD83D\\uDE0A\\uD83C\\uDFFB"));
	ck_assert(next() == NULL);
}
END_TEST


START_TEST(test_emoji_zwj_sequence)
{
	// \U0001F469\u200D\u2764\uFE0F\u200D\U0001F48B\u200D\U0001F469
	start(JS("\\ud83d\\udc69\\u200d\\u2764\\ufe0f\\u200d\\ud83d\\udc8b\\u200d\\ud83d\\udc69"));

	assert_text_eq(next(), JS("\\ud83d\\udc69\\u200d\\u2764\\ufe0f\\u200d\\ud83d\\udc8b\\u200d\\ud83d\\udc69"));

	ck_assert(next() == NULL);
}
END_TEST

// Check that isolated codepoints are single graphemes.
START_TEST(test_isolated)
{
	uint8_t buf[4];
	uint8_t *end;
	int32_t code;
	struct utf8lite_text text;

	for (code = 1; code <= 0x1FFF; code++) {
		if (!UTF8LITE_IS_UNICODE(code))
			continue;
		end = buf;
		utf8lite_encode_utf8(code, &end);
		utf8lite_text_assign(&text, buf, end - buf, 0, NULL);

		start(&text);
		assert_text_eq(next(), &text);
		ck_assert(next() == NULL);
	}
}
END_TEST

// Unicode Grapheme Break Test
// http://www.unicode.org/Public/UCD/latest/ucd/auxiliary/GraphemeBreakTest.txt
struct unitest {
	char comment[4096];
	unsigned line;
	int is_ascii;

	struct utf8lite_text text;
	uint8_t buf[4096];

	int32_t code[256];
	int can_break_before[256];
	uint8_t *code_end[256];
	unsigned ncode;

	uint8_t *break_begin[256];
	uint8_t *break_end[256];
	unsigned nbreak;

};

struct unitest unitests[4096];
unsigned nunitest;

void write_unitest(FILE *stream, const struct unitest *test)
{
	unsigned i, n = test->ncode;

	for (i = 0; i < n; i++) {
		fprintf(stream, "%s %04X ",
			(test->can_break_before[i]) ? "\xC3\xB7" : "\xC3\x97",
			test->code[i]);
	}
	fprintf(stream, "\xC3\xB7 %s\n", test->comment);
}

void setup_unicode(void)
{
	struct unitest *test;
	FILE *file;
	unsigned code, line, nbreak, ncode;
	uint8_t *dst;
	char *comment;
	int ch, is_ascii;

	setup_scan();
	file = fopen(GRAPH_BREAK_TEST, "r");
	if (!file) {
		file = fopen("../"GRAPH_BREAK_TEST, "r");
	}

	nunitest = 0;
	test = &unitests[0];

	line = 1;
	ncode = 0;
	nbreak = 0;
	is_ascii = 1;
	test->text.ptr = &test->buf[0];
	dst = test->text.ptr;

	ck_assert_msg(file != NULL, "file '"GRAPH_BREAK_TEST"' not found");
	while ((ch = fgetc(file)) != EOF) {
		switch (ch) {
		case '#':
			comment = &test->comment[0];
			do {
				*comment++ = (char)ch;
				ch = fgetc(file);
			} while (ch != EOF && ch != '\n');
			*comment = '\0';

			if (ch == EOF) {
				goto eof;
			}
			/* fallthrough */
		case '\n':
			*dst = '\0';

			test->line = line;
			test->is_ascii = is_ascii;
			test->text.attr = (size_t)(dst - test->text.ptr);

			if (ncode > 0) {
				test->ncode = ncode;
				test->nbreak = nbreak;
				ncode = 0;
				nbreak = 0;
				is_ascii = 1;
				nunitest++;
				test = &unitests[nunitest];
				test->text.ptr = &test->buf[0];
				test->comment[0] = '\0';
				dst = test->text.ptr;
			}
			line++;
			break;

		case 0xC3:
			ch = fgetc(file);
			if (ch == EOF) {
				goto eof;
			} else if (ch == 0x97) {
				// MULTIPLICATON SIGN (U+00D7) 0xC3 0x97
				test->can_break_before[ncode] = 0;
			} else if (ch == 0xB7) {
				// DIVISION SIGN (U+00F7) 0xC3 0xB7
				test->can_break_before[ncode] = 1;
			} else {
				goto inval;
			}

			if (test->can_break_before[ncode]) {
				test->break_begin[nbreak] = dst;
				if (nbreak > 0) {
					test->break_end[nbreak - 1] = dst;
				}
				nbreak++;
			}

			if (fscanf(file, "%x", &code)) {
				test->code[ncode] = (int32_t)code;
				if (code > 0x7F) {
					is_ascii = 0;
				}
				utf8lite_encode_utf8((int32_t)code, &dst);
				test->code_end[ncode] = dst;
				ncode++;
			} else {
				test->break_end[nbreak - 1] = dst;
				nbreak--;
			}
			break;
		}

	}
eof:
	fclose(file);
	return;
inval:
	fprintf(stderr, "invalid character on line %d\n", line);
	fclose(file);
}


void teardown_unicode(void)
{
	teardown_scan();
}


START_TEST(test_unicode_forward)
{
	struct unitest *test;
	unsigned i, j;

	for (i = 0; i < nunitest; i++) {
		test = &unitests[i];

		//fprintf(stderr, "[%u]: ", i);
		//write_unitest(stderr, test);
		utf8lite_graphscan_make(&scan, &test->text);

		for (j = 0; j < test->nbreak; j++) {
			//fprintf(stderr, "Break %u\n", j);
			ck_assert(utf8lite_graphscan_advance(&scan));
			ck_assert(scan.current.text.ptr
					== test->break_begin[j]);
			ck_assert(scan.current.text.ptr
				  + UTF8LITE_TEXT_SIZE(&scan.current.text)
				  == test->break_end[j]);
		}
		ck_assert(!utf8lite_graphscan_advance(&scan));
	}
}
END_TEST


START_TEST(test_unicode_backward)
{
	struct unitest *test;
	unsigned i, j;

	for (i = 0; i < nunitest; i++) {
		test = &unitests[i];

		//fprintf(stderr, "[%u]: ", i);
		//write_unitest(stderr, test);
		utf8lite_graphscan_make(&scan, &test->text);
		utf8lite_graphscan_skip(&scan);
		ck_assert(scan.current.text.ptr
				== test->break_end[test->nbreak]);
		ck_assert(scan.current.text.attr == 0);

		j = test->nbreak;
		while (j-- > 0) {
			//fprintf(stderr, "Break %u\n", j);
			ck_assert(utf8lite_graphscan_retreat(&scan));
			ck_assert(scan.current.text.ptr
					== test->break_begin[j]);
			ck_assert(scan.current.text.ptr
				  + UTF8LITE_TEXT_SIZE(&scan.current.text)
				  == test->break_end[j]);
		}
		//fprintf(stderr, "Start\n");
		ck_assert(!utf8lite_graphscan_retreat(&scan));
		ck_assert(!utf8lite_graphscan_retreat(&scan));
	}
}
END_TEST


Suite *graphscan_suite(void)
{
        Suite *s;
        TCase *tc;

        s = suite_create("graphscan");
	tc = tcase_create("core");
        tcase_add_checked_fixture(tc, setup_scan, teardown_scan);
        tcase_add_test(tc, test_empty);
        tcase_add_test(tc, test_single);
        tcase_add_test(tc, test_emoji_modifier);
	tcase_add_test(tc, test_emoji_zwj_sequence);
	tcase_add_test(tc, test_isolated);
        suite_add_tcase(s, tc);

        tc = tcase_create("Unicode GraphemeBreakTest.txt");
        tcase_add_checked_fixture(tc, setup_unicode, teardown_unicode);
        tcase_add_test(tc, test_unicode_forward);
        tcase_add_test(tc, test_unicode_backward);
        suite_add_tcase(s, tc);

	return s;
}


int main(void)
{
        int number_failed;
        Suite *s;
        SRunner *sr;

        s = graphscan_suite();
        sr = srunner_create(s);

        srunner_run_all(sr, CK_NORMAL);
        number_failed = srunner_ntests_failed(sr);
        srunner_free(sr);

        return (number_failed == 0) ? EXIT_SUCCESS : EXIT_FAILURE;
}
