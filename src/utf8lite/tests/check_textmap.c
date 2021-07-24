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


#define TEXTMAP_CASE	UTF8LITE_TEXTMAP_CASE
#define TEXTMAP_COMPAT	UTF8LITE_TEXTMAP_COMPAT
#define TEXTMAP_QUOTE	UTF8LITE_TEXTMAP_QUOTE
#define TEXTMAP_RMDI	UTF8LITE_TEXTMAP_RMDI


struct utf8lite_text *get_map(const struct utf8lite_text *text, int flags)
{
	struct utf8lite_text *val;
	struct utf8lite_textmap map;
	size_t size;

	ck_assert(!utf8lite_textmap_init(&map, flags));
	ck_assert(!utf8lite_textmap_set(&map, text));

	size = UTF8LITE_TEXT_SIZE(&map.text);
	val = alloc(sizeof(*val));

	val->ptr = alloc(size + 1);
	memcpy(val->ptr, map.text.ptr, size);
	val->ptr[size] = '\0';
	val->attr = map.text.attr;

	utf8lite_textmap_destroy(&map);
	return val;
}


struct utf8lite_text *casefold(const struct utf8lite_text *text)
{
	return get_map(text, TEXTMAP_CASE);
}


START_TEST(test_map_basic)
{
	assert_text_eq(get_map(S("hello"), 0), S("hello"));
	assert_text_eq(get_map(S("world"), 0), JS("world"));
	assert_text_eq(get_map(JS("foo"), 0), S("foo"));
}
END_TEST


START_TEST(test_map_esc)
{
	// backslash
	assert_text_eq(get_map(S("\\"), 0), S("\\"));
	assert_text_eq(get_map(JS("\\\\"), 0), S("\\"));
	assert_text_eq(get_map(JS("\\u005C"), 0), S("\\"));
	assert_text_eq(get_map(S("\\\\"), 0), S("\\\\"));
	assert_text_eq(get_map(S("\\u005C"), TEXTMAP_CASE), S("\\u005c"));

	// quote (')
	assert_text_eq(get_map(S("'"), TEXTMAP_QUOTE), S("'"));
	assert_text_eq(get_map(JS("'"), TEXTMAP_QUOTE), S("'"));
	assert_text_eq(get_map(S("\""), TEXTMAP_QUOTE), S("\""));
	assert_text_eq(get_map(JS("\\\""), TEXTMAP_QUOTE), S("\""));
	assert_text_eq(get_map(JS("\\u2019"), TEXTMAP_QUOTE), S("\'"));
	//assert_text_eq(get_map(JS("\\u201c"), TEXTMAP_QUOTE), S("\""));
	assert_text_eq(get_map(S("\\\'"), TEXTMAP_QUOTE), S("\\\'"));
	assert_text_eq(get_map(S("\\u2019"), TEXTMAP_QUOTE), S("\\u2019"));
}
END_TEST


START_TEST(test_keep_control_ascii)
{
	const struct utf8lite_text *js, *t;
	char str[256];
	uint8_t i;

	assert_text_eq(get_map(S("\a"), 0), S("\a"));
	assert_text_eq(get_map(S("\b"), 0), S("\b"));

	// C0
	for (i = 1; i < 0x20; i++) {
		if (0x09 <= i && i <= 0x0D) {
			continue;
		}
		str[0] = (char)i; str[1] = '\0';
		t = S(str);
		assert_text_eq(get_map(t, 0), t);

		sprintf(str, "\\u%04X", i);
		js = JS(str);
		assert_text_eq(get_map(js, 0), t);
	}

	// delete
	assert_text_eq(get_map(S("\x7F"), 0), S("\x7F"));
	assert_text_eq(get_map(JS("\\u007F"), 0), S("\x7F"));
}
END_TEST


START_TEST(test_keep_control_utf8)
{
	const struct utf8lite_text *t, *js;
	uint8_t str[256];
	uint8_t i;

	// C1
	for (i = 0x80; i < 0xA0; i++) {
		if (i == 0x85) {
			continue;
		}

		str[0] = 0xC2; str[1] = i; str[2] = '\0';
		t = S((char *)str);
		assert_text_eq(get_map(t, 0), t);

		sprintf((char *)str, "\\u%04X", i);
		js = JS((char *)str);
		assert_text_eq(get_map(js, 0), t);
	}
}
END_TEST


START_TEST(test_keep_ws_ascii)
{
	assert_text_eq(get_map(S("\t"), 0), S("\t"));
	assert_text_eq(get_map(S("\n"), 0), S("\n"));
	assert_text_eq(get_map(S("\v"), 0), S("\v"));
	assert_text_eq(get_map(S("\f"), 0), S("\f"));
	assert_text_eq(get_map(S("\r"), 0), S("\r"));
	assert_text_eq(get_map(S(" "), 0), S(" "));
}
END_TEST


START_TEST(test_keep_ws_utf8)
{
	const struct utf8lite_text *t, *js, *text;
	uint8_t str[256];
	uint8_t *buf;
	unsigned ws[] = { 0x0009, 0x000A, 0x000B, 0x000C, 0x000D,
			  0x0020, 0x0085, 0x00A0, 0x1680, 0x2000,
			  0x2001, 0x2002, 0x2003, 0x2004, 0x2005,
			  0x2006, 0x2007, 0x2008, 0x2009, 0x200A,
			  0x2028, 0x2029, 0x202F, 0x205F, 0x3000 };
	int i, n = sizeof(ws) / sizeof(ws[0]);

	for (i = 0; i < n; i++) {
		//fprintf(stderr, "i = %d; ws = U+%04X\n", i, ws[i]);

		switch (ws[i]) {
		case 0x0009:
			text = S("\t");
			break;
		case 0x000A:
			text = S("\n");
			break;
		case 0x000B:
			text = S("\v");
			break;
		case 0x000C:
			text = S("\f");
			break;
		case 0x000D:
			text = S("\r");
			break;
		case 0x0085: // NEXT LINE (NEL)
			text = S("\xC2\x85");
			break;
		case 0x1680: // OGHAM SPACE MARK
			text = S("\xE1\x9A\x80");
			break;
		case 0x2028: // LINE SEPARATOR
			text = S("\xE2\x80\xA8");
			break;
		case 0x2029: // PARAGRAPH SEPARATOR
			text = S("\xE2\x80\xA9");
			break;
		default:
			text = S(" ");
			break;
		}

		buf = str;
		utf8lite_encode_utf8(ws[i], &buf);
		*buf = '\0';
		t = S((char *)str);
		assert_text_eq(get_map(t, TEXTMAP_COMPAT), text);

		sprintf((char *)str, "\\u%04x", ws[i]);
		js = JS((char *)str);
		assert_text_eq(get_map(js, TEXTMAP_COMPAT), text);
	}
}
END_TEST


// removed the following features from textmap, no need to test
#if 0

/*
 * Control Characters (Cc)
 * -----------------------
 *
 *	U+0000..U+001F	(C0)
 *	U+007F		(delete)
 *	U+0080..U+009F	(C1)
 *
 * Source: UnicodeStandard-8.0, Sec. 23.1, p. 808.
 */

START_TEST(test_rm_control_ascii)
{
	char str[256];
	uint8_t i;

	assert_text_eq(get_map(S("\a"), TYPE_RMCC), S(""));
	assert_text_eq(get_map(S("\b"), TYPE_RMCC), S(""));
	assert_text_eq(get_map(S("\t"), TYPE_RMCC), S("\t"));
	assert_text_eq(get_map(S("\n"), TYPE_RMCC), S("\n"));
	assert_text_eq(get_map(S("\v"), TYPE_RMCC), S("\v"));
	assert_text_eq(get_map(S("\f"), TYPE_RMCC), S("\f"));
	assert_text_eq(get_map(S("\r"), TYPE_RMCC), S("\r"));

	// C0
	for (i = 1; i < 0x20; i++) {
		if (0x09 <= i && i <= 0x0D) {
			continue;
		}

		str[0] = (char)i; str[1] = '\0';
		assert_text_eq(get_map(S(str), TYPE_RMCC), S(""));

		sprintf(str, "\\u%04X", i);
		assert_text_eq(get_map(JS(str), TYPE_RMCC), S(""));
	}

	// delete
	assert_text_eq(get_map(S("\x7F"), TYPE_RMCC), S(""));
	assert_text_eq(get_map(JS("\\u007F"), TYPE_RMCC), S(""));
}
END_TEST


START_TEST(test_rm_control_utf8)
{
	uint8_t str[256];
	uint8_t i;

	// C1: JSON
	for (i = 0x80; i < 0xA0; i++) {
		if (i == 0x85) {
			continue;
		}

		str[0] = 0xC2; str[1] = i; str[2] = '\0';
		assert_text_eq(get_map(S((char *)str), TYPE_RMCC), S(""));

		sprintf((char *)str, "\\u%04X", i);
		assert_text_eq(get_map(JS((char *)str), TYPE_RMCC), S(""));
	}
}
END_TEST


START_TEST(test_rm_ws_ascii)
{
	assert_text_eq(get_map(S("\t"), TYPE_RMWS), S(""));
	assert_text_eq(get_map(S("\n"), TYPE_RMWS), S(""));
	assert_text_eq(get_map(S("\v"), TYPE_RMWS), S(""));
	assert_text_eq(get_map(S("\f"), TYPE_RMWS), S(""));
	assert_text_eq(get_map(S("\r"), TYPE_RMWS), S(""));
	assert_text_eq(get_map(S(" "), TYPE_RMWS), S(""));
}
END_TEST


START_TEST(test_rm_ws_utf8)
{
	const struct utf8lite_text *t, *js;
	uint8_t str[256];
	uint8_t *buf;
	uint32_t ws[] = { 0x0009, 0x000A, 0x000B, 0x000C, 0x000D,
			  0x0020, 0x0085, 0x00A0, 0x1680, 0x2000,
			  0x2001, 0x2002, 0x2003, 0x2004, 0x2005,
			  0x2006, 0x2007, 0x2008, 0x2009, 0x200A,
			  0x2028, 0x2029, 0x202F, 0x205F, 0x3000 };
	int i, n = sizeof(ws) / sizeof(ws[0]);

	for (i = 0; i < n; i++) {
		buf = str;
		utf8lite_encode_utf8(ws[i], &buf);
		*buf = '\0';
		t = S((char *)str);
		assert_text_eq(get_map(t, TYPE_RMWS), S(""));

		sprintf((char *)str, "\\u%04x", ws[i]);
		js = JS((char *)str);
		assert_text_eq(get_map(js, TYPE_RMWS), S(""));
	}
}
END_TEST

#endif

START_TEST(test_casefold_ascii)
{
	const struct utf8lite_text *text;
	uint8_t buf[2] = { 0, 0 };
	uint8_t i;

	assert_text_eq(casefold(S("UPPER CASE")), S("upper case"));
	assert_text_eq(casefold(S("lower case")), S("lower case"));
	assert_text_eq(casefold(S("mIxEd CaSe")), S("mixed case"));

	for (i = 0x01; i < 'A'; i++) {
		buf[0] = i;
		text = S((char *)buf);
		assert_text_eq(casefold(text), text);
	}
	for (i = 'Z' + 1; i < 0x7F; i++) {
		buf[0] = i;
		text = S((char *)buf);
		assert_text_eq(casefold(text), text);
	}

	// upper
	assert_text_eq(casefold(S("ABCDEFGHIJKLMNOPQRSTUVWXYZ")),
		       S("abcdefghijklmnopqrstuvwxyz"));

	// lower
	assert_text_eq(casefold(S("abcdefghijklmnopqrstuvwxyz")),
		       S("abcdefghijklmnopqrstuvwxyz"));

	// digit
	assert_text_eq(casefold(S("0123456789")), S("0123456789"));

	// punct
	assert_text_eq(casefold(S("!\"#$%&'()*+,-./:;<=>?@[\\]^_`{|}~")),
		       S("!\"#$%&'()*+,-./:;<=>?@[\\]^_`{|}~"));

	// space
	assert_text_eq(casefold(S("\t\n\v\f\r ")), S("\t\n\v\f\r "));
}
END_TEST


START_TEST(test_casefold_utf8)
{
	assert_text_eq(casefold(JS("\u1e9e")), JS("ss")); // capital eszett
	assert_text_eq(casefold(JS("\u00df")), JS("ss")); // lowercase eszett
}
END_TEST


// removed this feature
#if 0

START_TEST(test_fold_dash)
{
	assert_text_eq(get_map(S("-"), TYPE_DASHFOLD), S("-"));
	assert_text_eq(get_map(JS("\\u058A"), TYPE_DASHFOLD), S("-"));
	assert_text_eq(get_map(JS("\\u2212"), TYPE_DASHFOLD), S("-"));
	assert_text_eq(get_map(JS("\\u2E3A"), TYPE_DASHFOLD), S("-"));
	assert_text_eq(get_map(JS("\\u2E3B"), TYPE_DASHFOLD), S("-"));
	assert_text_eq(get_map(JS("\\uFF0D"), TYPE_DASHFOLD), S("-"));
}
END_TEST


START_TEST(test_nofold_dash)
{
	assert_text_eq(get_map(S("-"), 0), S("-"));
	assert_text_eq(get_map(JS("\\u058A"), 0), S("\xD6\x8A"));
	assert_text_eq(get_map(JS("\\u2212"), 0), S("\xE2\x88\x92"));
	assert_text_eq(get_map(JS("\\u2E3A"), 0), S("\xE2\xB8\xBA"));
	assert_text_eq(get_map(JS("\\u2E3B"), 0), S("\xE2\xB8\xBB"));
	assert_text_eq(get_map(JS("\\uFF0D"), 0), S("\xEF\xBC\x8D"));
}
END_TEST

#endif


START_TEST(test_map_quote)
{
	assert_text_eq(get_map(S("'"), TEXTMAP_QUOTE), S("'"));
	assert_text_eq(get_map(S("\""), TEXTMAP_QUOTE), S("\""));
	assert_text_eq(get_map(JS("\\u2018"), TEXTMAP_QUOTE), S("'"));
	assert_text_eq(get_map(JS("\\u2019"), TEXTMAP_QUOTE), S("'"));
	//assert_text_eq(get_map(JS("\\u201C"), TEXTMAP_QUOTE), S("\""));
	//assert_text_eq(get_map(JS("\\u201D"), TEXTMAP_QUOTE), S("\""));
}
END_TEST


START_TEST(test_nomap_quote)
{
	assert_text_eq(get_map(S("'"), 0), S("'"));
	assert_text_eq(get_map(S("\""), 0), S("\""));
	assert_text_eq(get_map(JS("\\u2018"), 0), S("\xE2\x80\x98"));
	assert_text_eq(get_map(JS("\\u2019"), 0), S("\xE2\x80\x99"));
	assert_text_eq(get_map(JS("\\u201A"), 0), S("\xE2\x80\x9A"));
	assert_text_eq(get_map(JS("\\u201F"), 0), S("\xE2\x80\x9F"));
}
END_TEST


Suite *textmap_suite(void)
{
	Suite *s;
	TCase *tc;

	s = suite_create("textmap");
	tc = tcase_create("normalize");
	tcase_add_checked_fixture(tc, setup, teardown);
	tcase_add_test(tc, test_map_basic);
	tcase_add_test(tc, test_map_esc);
	// tcase_add_test(tc, test_rm_control_ascii);
	tcase_add_test(tc, test_keep_control_ascii);
	// tcase_add_test(tc, test_rm_control_utf8);
	tcase_add_test(tc, test_keep_control_utf8);
	// tcase_add_test(tc, test_rm_ws_ascii);
	tcase_add_test(tc, test_keep_ws_ascii);
	// tcase_add_test(tc, test_rm_ws_utf8);
	tcase_add_test(tc, test_keep_ws_utf8);
	tcase_add_test(tc, test_casefold_ascii);
	tcase_add_test(tc, test_casefold_utf8);
	// tcase_add_test(tc, test_fold_dash);
	// tcase_add_test(tc, test_nofold_dash);
	tcase_add_test(tc, test_map_quote);
	tcase_add_test(tc, test_nomap_quote);
	suite_add_tcase(s, tc);

	return s;
}


int main(void)
{
        int number_failed;
        Suite *s;
        SRunner *sr;

        s = textmap_suite();
        sr = srunner_create(s);

        srunner_run_all(sr, CK_NORMAL);
        number_failed = srunner_ntests_failed(sr);
        srunner_free(sr);

        return (number_failed == 0) ? EXIT_SUCCESS : EXIT_FAILURE;
}
