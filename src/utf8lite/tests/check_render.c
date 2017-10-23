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

#include <check.h>
#include <stdio.h>
#include <stdlib.h>
#include "../src/utf8lite.h"
#include "testutil.h"


struct utf8lite_render render;


void setup_render(void)
{
        setup();
	ck_assert(!utf8lite_render_init(&render, 0));
}


void teardown_render(void)
{
	utf8lite_render_destroy(&render);
        teardown();
}

struct char_test {
	const char *raw;
	const char *c;
	const char *json;
};

START_TEST(test_escape_control)
{
	const struct char_test tests[] = {
		{ "\x01", "\\u0001", "\\u0001" },
		{ "\a", "\\a", "\\u0007" },
		{ "\b", "\\b", "\\b" },
		{ "\f", "\\f", "\\f" },
		{ "\n", "\\n", "\\n" },
		{ "\r", "\\r", "\\r" },
		{ "\t", "\\t", "\\t" },
		{ "\v", "\\v", "\\u000b" },
		{ "\x7F", "\\u007f", "\\u007f" },
		{ "\xC2\x80", "\\u0080", "\\u0080"},
		{ "\xC2\x9F", "\\u009f", "\\u009f"},
		{ "\xE0\xB8\x80", "\\u0e00", "\\u0e00" },
		{ "\xE2\x80\xA9", "\\u2029", "\\u2029" },
		{ "\xF4\x8F\xBF\xBF", "\\U0010ffff", "\\udbff\\udfff" }
	};
	int flag = UTF8LITE_ESCAPE_CONTROL;
	int i, n = sizeof(tests) / sizeof(tests[0]);


	for (i = 0; i < n; i++) {
		utf8lite_render_set_flags(&render, 0);
		ck_assert(!utf8lite_render_string(&render, tests[i].raw));
		ck_assert_str_eq(render.string, tests[i].raw);
		utf8lite_render_clear(&render);

		utf8lite_render_set_flags(&render, flag | UTF8LITE_ENCODE_C);
		ck_assert(!utf8lite_render_string(&render, tests[i].raw));
		ck_assert_str_eq(render.string, tests[i].c);
		utf8lite_render_clear(&render);

		utf8lite_render_set_flags(&render, flag | UTF8LITE_ENCODE_JSON);
		ck_assert(!utf8lite_render_string(&render, tests[i].raw));
		ck_assert_str_eq(render.string, tests[i].json);
		utf8lite_render_clear(&render);
	}
}
END_TEST

START_TEST(test_escape_dquote)
{
	utf8lite_render_set_flags(&render, 0);
	ck_assert(!utf8lite_render_char(&render, '\''));
	ck_assert_str_eq(render.string, "\'");
	utf8lite_render_clear(&render);

	utf8lite_render_set_flags(&render, UTF8LITE_ESCAPE_DQUOTE);
	ck_assert(!utf8lite_render_char(&render, '\"'));
	ck_assert_str_eq(render.string, "\\\"");
	utf8lite_render_clear(&render);
}
END_TEST


START_TEST(test_escape_squote)
{
	utf8lite_render_set_flags(&render, 0);
	ck_assert(!utf8lite_render_char(&render, '\''));
	ck_assert_str_eq(render.string, "\'");
	utf8lite_render_clear(&render);

	utf8lite_render_set_flags(&render, UTF8LITE_ESCAPE_SQUOTE);
	ck_assert(!utf8lite_render_char(&render, '\''));
	ck_assert_str_eq(render.string, "\\\'");
	utf8lite_render_clear(&render);
}
END_TEST


START_TEST(test_escape_backslash)
{
	int flags[] = {
		UTF8LITE_ESCAPE_CONTROL,
		UTF8LITE_ESCAPE_DQUOTE,
		UTF8LITE_ESCAPE_SQUOTE,
		UTF8LITE_ESCAPE_EXTENDED,
		UTF8LITE_ESCAPE_UTF8
	};
	int i, n = sizeof(flags) / sizeof(flags[0]);

	utf8lite_render_set_flags(&render, 0);
	ck_assert(!utf8lite_render_char(&render, '\\'));
	ck_assert_str_eq(render.string, "\\");
	utf8lite_render_clear(&render);

	for (i = 0; i < n; i++) {
		utf8lite_render_set_flags(&render, flags[i]);
		ck_assert(!utf8lite_render_char(&render, '\\'));
		ck_assert_str_eq(render.string, "\\\\");
		utf8lite_render_clear(&render);
	}
}
END_TEST


START_TEST(test_escape_extended)
{
	const struct char_test tests[] = {
		{ "\x01", "\x01", "\x01" },
		{ "\x20", "\x20", "\x20" },
		{ "\x7E", "\x7E", "\x7E" },
		{ "\x7F", "\x7F", "\x7F" },
		{ "\xC2\x80", "\xC2\x80", "\xC2\x80" },
		{ "\xC2\xA0", "\xC2\xA0", "\xC2\xA0" },
		{ "\xEF\xBF\xBD", "\xEF\xBF\xBD", "\xEF\xBF\xBD" },
		{ "\xEF\xBF\xBF", "\xEF\xBF\xBF", "\xEF\xBF\xBF" },
		{ "\xF0\x90\x80\x80", "\\U00010000", "\\ud800\\udc00" },
		{ "\xF0\xAF\xA8\x9D", "\\U0002fa1d", "\\ud87e\\ude1d" },
		{ "\xF4\x8F\xBF\xBF", "\\U0010ffff", "\\udbff\\udfff" }
	};
	int flag = UTF8LITE_ESCAPE_EXTENDED;
	int i, n = sizeof(tests) / sizeof(tests[0]);

	for (i = 0; i < n; i++) {
		utf8lite_render_set_flags(&render, 0);
		ck_assert(!utf8lite_render_string(&render, tests[i].raw));
		ck_assert_str_eq(render.string, tests[i].raw);
		utf8lite_render_clear(&render);

		utf8lite_render_set_flags(&render, flag | UTF8LITE_ENCODE_C);
		ck_assert(!utf8lite_render_string(&render, tests[i].raw));
		ck_assert_str_eq(render.string, tests[i].c);
		utf8lite_render_clear(&render);

		utf8lite_render_set_flags(&render, flag | UTF8LITE_ENCODE_JSON);
		ck_assert(!utf8lite_render_string(&render, tests[i].raw));
		ck_assert_str_eq(render.string, tests[i].json);
		utf8lite_render_clear(&render);
	}
}
END_TEST


START_TEST(test_escape_utf8)
{
	const struct char_test tests[] = {
		{ "\x01", "\x01", "\x01" },
		{ "\x20", "\x20", "\x20" },
		{ "\x7E", "\x7E", "\x7E" },
		{ "\x7F", "\x7F", "\x7F" },
		{ "\xC2\x80", "\\u0080", "\\u0080" },
		{ "\xC2\xA0", "\\u00a0", "\\u00a0" },
		{ "\xEF\xBF\xBD", "\\ufffd", "\\ufffd" },
		{ "\xEF\xBF\xBF", "\\uffff", "\\uffff" },
		{ "\xF0\x90\x80\x80", "\\U00010000", "\\ud800\\udc00" },
		{ "\xF0\xAF\xA8\x9D", "\\U0002fa1d", "\\ud87e\\ude1d" },
		{ "\xF4\x8F\xBF\xBF", "\\U0010ffff", "\\udbff\\udfff" }
	};
	int flag = UTF8LITE_ESCAPE_UTF8;
	int i, n = sizeof(tests) / sizeof(tests[0]);

	for (i = 0; i < n; i++) {
		utf8lite_render_set_flags(&render, 0);
		ck_assert(!utf8lite_render_string(&render, tests[i].raw));
		ck_assert_str_eq(render.string, tests[i].raw);
		utf8lite_render_clear(&render);

		utf8lite_render_set_flags(&render, flag | UTF8LITE_ENCODE_C);
		ck_assert(!utf8lite_render_string(&render, tests[i].raw));
		ck_assert_str_eq(render.string, tests[i].c);
		utf8lite_render_clear(&render);

		utf8lite_render_set_flags(&render, flag | UTF8LITE_ENCODE_JSON);
		ck_assert(!utf8lite_render_string(&render, tests[i].raw));
		ck_assert_str_eq(render.string, tests[i].json);
		utf8lite_render_clear(&render);
	}
}
END_TEST


Suite *render_suite(void)
{
        Suite *s;
        TCase *tc;

        s = suite_create("render");

	tc = tcase_create("escape");
        tcase_add_checked_fixture(tc, setup_render, teardown_render);
        tcase_add_test(tc, test_escape_control);
        tcase_add_test(tc, test_escape_dquote);
        tcase_add_test(tc, test_escape_squote);
        tcase_add_test(tc, test_escape_backslash);
        tcase_add_test(tc, test_escape_extended);
        tcase_add_test(tc, test_escape_utf8);
        suite_add_tcase(s, tc);

	return s;
}


int main(void)
{
        int number_failed;
        Suite *s;
        SRunner *sr;

        s = render_suite();
        sr = srunner_create(s);

        srunner_run_all(sr, CK_NORMAL);
        number_failed = srunner_ntests_failed(sr);
        srunner_free(sr);

        return (number_failed == 0) ? EXIT_SUCCESS : EXIT_FAILURE;
}
