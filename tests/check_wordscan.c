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

#define WORD_BREAK_TEST "data/ucd/auxiliary/WordBreakTest.txt"
struct utf8lite_wordscan scan;


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
	utf8lite_wordscan_make(&scan, text);
}


const struct utf8lite_text *next(void)
{
	struct utf8lite_text *word;
	if (!utf8lite_wordscan_advance(&scan)) {
		return NULL;
	}
	word = alloc(sizeof(*word));
	*word = scan.current;
	return word;
}


START_TEST(test_figure1)
{
	// Test Figure 1 from http://www.unicode.org/reports/tr29/
	start(S("The quick (\"brown\") fox can't jump 32.3 feet, right?"));
	assert_text_eq(next(), S("The"));
	assert_text_eq(next(), S(" "));
	assert_text_eq(next(), S("quick"));
	assert_text_eq(next(), S(" "));
	assert_text_eq(next(), S("("));
	assert_text_eq(next(), S("\""));
	assert_text_eq(next(), S("brown"));
	assert_text_eq(next(), S("\""));
	assert_text_eq(next(), S(")"));
	assert_text_eq(next(), S(" "));
	assert_text_eq(next(), S("fox"));
	assert_text_eq(next(), S(" "));
	assert_text_eq(next(), S("can't"));
	assert_text_eq(next(), S(" "));
	assert_text_eq(next(), S("jump"));
	assert_text_eq(next(), S(" "));
	assert_text_eq(next(), S("32.3"));
	assert_text_eq(next(), S(" "));
	assert_text_eq(next(), S("feet"));
	assert_text_eq(next(), S(","));
	assert_text_eq(next(), S(" "));
	assert_text_eq(next(), S("right"));
	assert_text_eq(next(), S("?"));
	ck_assert(next() == NULL);
}
END_TEST

START_TEST(test_quote)
{
	start(S("both 'single' and \"double\"."));
	assert_text_eq(next(), S("both"));
	assert_text_eq(next(), S(" "));
	assert_text_eq(next(), S("'"));
	assert_text_eq(next(), S("single"));
	assert_text_eq(next(), S("'"));
	assert_text_eq(next(), S(" "));
	assert_text_eq(next(), S("and"));
	assert_text_eq(next(), S(" "));
	assert_text_eq(next(), S("\""));
	assert_text_eq(next(), S("double"));
	assert_text_eq(next(), S("\""));
	assert_text_eq(next(), S("."));
	ck_assert(next() == NULL);
}
END_TEST


START_TEST(test_extendnumlet)
{
	start(S("_"));
	assert_text_eq(next(), S("_"));

	start(S("__"));
	assert_text_eq(next(), S("__"));

	start(S("___"));
	assert_text_eq(next(), S("___"));

	start(JS("\\u202f"));
	assert_text_eq(next(), JS("\\u202f"));

	start(JS("\\u202f\\u202f"));
	assert_text_eq(next(), JS("\\u202f\\u202f"));

	start(JS("\\u202f_"));
	assert_text_eq(next(), JS("\\u202f_"));

	start(S("_1"));
	assert_text_eq(next(), S("_1"));

	start(S("__1"));
	assert_text_eq(next(), S("__1"));

	start(S("_A"));
	assert_text_eq(next(), S("_A"));

	start(S("__A"));
	assert_text_eq(next(), S("__A"));
}
END_TEST


// Unicode Word Break Test
// http://www.unicode.org/Public/UCD/latest/ucd/auxiliary/WordBreakTest.txt
struct unitest {
	char comment[1024];
	unsigned line;
	int is_ascii;

	struct utf8lite_text text;
	uint8_t buf[1024];

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
	file = fopen(WORD_BREAK_TEST, "r");
	if (!file) {
		file = fopen("../"WORD_BREAK_TEST, "r");
	}

	nunitest = 0;
	test = &unitests[0];

	line = 1;
	ncode = 0;
	nbreak = 0;
	is_ascii = 1;
	test->text.ptr = &test->buf[0];
	dst = test->text.ptr;

	ck_assert_msg(file != NULL, "file '"WORD_BREAK_TEST"' not found");
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
				comment = &test->comment[0];
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
	return;
inval:
	fprintf(stderr, "invalid character on line %d\n", line);

	fclose(file);
}

void teardown_unicode(void)
{
	teardown_scan();
}

START_TEST(test_unicode)
{
	struct unitest *test;
	unsigned i, j;

	for (i = 0; i < nunitest; i++) {
		test = &unitests[i];

		//write_unitest(stderr, test);
		utf8lite_wordscan_make(&scan, &test->text);

		for (j = 0; j < test->nbreak; j++) {
			//fprintf(stderr, "Break %u\n", j);
			ck_assert(utf8lite_wordscan_advance(&scan));
			ck_assert(scan.current.ptr == test->break_begin[j]);
			ck_assert(scan.current.ptr
					+ UTF8LITE_TEXT_SIZE(&scan.current)
					== test->break_end[j]);
		}
		ck_assert(!utf8lite_wordscan_advance(&scan));
	}
}
END_TEST

Suite *wordscan_suite(void)
{
        Suite *s;
        TCase *tc;

        s = suite_create("wordscan");
	tc = tcase_create("core");
        tcase_add_checked_fixture(tc, setup_scan, teardown_scan);
        tcase_add_test(tc, test_figure1);
        tcase_add_test(tc, test_quote);
        tcase_add_test(tc, test_extendnumlet);
        suite_add_tcase(s, tc);

        tc = tcase_create("Unicode WordBreakTest.txt");
        tcase_add_checked_fixture(tc, setup_unicode, teardown_unicode);
        tcase_add_test(tc, test_unicode);
        suite_add_tcase(s, tc);

	return s;
}


int main(void)
{
        int number_failed;
        Suite *s;
        SRunner *sr;

        s = wordscan_suite();
        sr = srunner_create(s);

        srunner_run_all(sr, CK_NORMAL);
        number_failed = srunner_ntests_failed(sr);
        srunner_free(sr);

        return (number_failed == 0) ? EXIT_SUCCESS : EXIT_FAILURE;
}
