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
#include <limits.h>
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


static void clear(void)
{
	utf8lite_render_clear(&render);
}


static void spaces(int n)
{
	ck_assert(!utf8lite_render_chars(&render, ' ', n));
}


static void newlines(int n)
{
	ck_assert(!utf8lite_render_newlines(&render, n));
}


static void indent(int n)
{
	ck_assert(!utf8lite_render_indent(&render, n));
}


static void set_flags(int flags)
{
	ck_assert(!utf8lite_render_set_flags(&render, flags));
}


static void set_newline(const char *newline)
{
	ck_assert(!utf8lite_render_set_newline(&render, newline));
}


static void set_tab(const char *tab)
{
	ck_assert(!utf8lite_render_set_tab(&render, tab));
}


static int width(const struct utf8lite_text *text)
{
	struct utf8lite_graphscan scan;
	int width, w;

	width = 0;
	utf8lite_graphscan_make(&scan, text);
	while (utf8lite_graphscan_advance(&scan)) {
		ck_assert(!utf8lite_graph_measure(&scan.current,
						  render.flags, &w));
		if (w == -1) {
			return w;
		}
		ck_assert(w >= 0);
		ck_assert(width <= INT_MAX - w);
		width += w;
	}

	return width;
}


START_TEST(test_format_spaces)
{
	spaces(-1);
	ck_assert_str_eq(render.string, "");

	spaces(0);
	ck_assert_str_eq(render.string, "");

	spaces(1);
	ck_assert_str_eq(render.string, " ");
	clear();

	spaces(2);
	ck_assert_str_eq(render.string, "  ");
	clear();

	spaces(3);
	ck_assert_str_eq(render.string, "   ");
	clear();
}
END_TEST


START_TEST(test_format_newlines)
{
	newlines(-1);
	ck_assert_str_eq(render.string, "");

	newlines(0);
	ck_assert_str_eq(render.string, "");

	newlines(1);
	ck_assert_str_eq(render.string, "\n");
	clear();

	newlines(2);
	ck_assert_str_eq(render.string, "\n\n");
	clear();

	newlines(3);
	ck_assert_str_eq(render.string, "\n\n\n");
	clear();
}
END_TEST


START_TEST(test_format_newlines_custom)
{
	set_newline("<LF>");

	newlines(-1);
	ck_assert_str_eq(render.string, "");

	newlines(0);
	ck_assert_str_eq(render.string, "");

	newlines(1);
	ck_assert_str_eq(render.string, "<LF>");
	clear();

	newlines(2);
	ck_assert_str_eq(render.string, "<LF><LF>");
	clear();

	newlines(3);
	ck_assert_str_eq(render.string, "<LF><LF><LF>");
	clear();
}
END_TEST


START_TEST(test_format_indent)
{
	indent(-1);
	ck_assert_str_eq(render.string, "");
	ck_assert(!utf8lite_render_string(&render, "I. "));
	ck_assert_str_eq(render.string, "I. ");

	indent(2);
	ck_assert(!utf8lite_render_string(&render, "Level 1"));
	ck_assert_str_eq(render.string, "I. Level 1");
	newlines(1);

	ck_assert(!utf8lite_render_string(&render, "A. Level 2"));
	ck_assert_str_eq(render.string, "I. Level 1\n\t\tA. Level 2");

	newlines(1);
	indent(-1);
	ck_assert(!utf8lite_render_string(&render, "B."));
	ck_assert_str_eq(render.string, "I. Level 1\n\t\tA. Level 2\n\tB.");

	indent(-2);
	newlines(1);
	ck_assert(!utf8lite_render_string(&render, "II."));
	ck_assert_str_eq(render.string,
			 "I. Level 1\n\t\tA. Level 2\n\tB.\nII.");
}
END_TEST


START_TEST(test_format_indent_custom)
{
	set_tab("<TAB>");

	ck_assert(!utf8lite_render_string(&render, "I"));

	newlines(1);
	indent(1);
	ck_assert(!utf8lite_render_string(&render, "A"));

	newlines(1);
	indent(1);
	ck_assert(!utf8lite_render_string(&render, "1"));

	newlines(1);
	ck_assert(!utf8lite_render_string(&render, "2"));

	indent(-1);
	newlines(1);
	ck_assert(!utf8lite_render_string(&render, "B"));

	newlines(1);
	ck_assert(!utf8lite_render_string(&render, "C"));
	newlines(1);

	ck_assert_str_eq(render.string,
			"I\n<TAB>A\n<TAB><TAB>1\n<TAB><TAB>2\n"
			"<TAB>B\n<TAB>C\n");
}
END_TEST


START_TEST(test_format_printf)
{
	ck_assert(!utf8lite_render_printf(&render, "%s", ""));
	ck_assert_str_eq(render.string, "");
	clear();

	ck_assert(!utf8lite_render_printf(&render, "%s %d", "hello", 99));
	ck_assert_str_eq(render.string, "hello 99");
	clear();

	ck_assert(!utf8lite_render_printf(&render, "%s", "\n"));
	ck_assert_str_eq(render.string, "\n");
	clear();

	set_flags(UTF8LITE_ESCAPE_CONTROL);
	ck_assert(!utf8lite_render_printf(&render, "%s", "\n"));
	ck_assert_str_eq(render.string, "\\n");
	clear();
}
END_TEST


struct escape_test {
	const char *raw;
	const char *c;
	const char *json;
};

START_TEST(test_escape_control)
{
	const struct escape_test tests[] = {
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
		set_flags(0);
		ck_assert(!utf8lite_render_string(&render, tests[i].raw));
		ck_assert_str_eq(render.string, tests[i].raw);
		utf8lite_render_clear(&render);

		set_flags(flag | UTF8LITE_ENCODE_C);
		ck_assert(!utf8lite_render_string(&render, tests[i].raw));
		ck_assert_str_eq(render.string, tests[i].c);
		utf8lite_render_clear(&render);

		set_flags(flag | UTF8LITE_ENCODE_JSON);
		ck_assert(!utf8lite_render_string(&render, tests[i].raw));
		ck_assert_str_eq(render.string, tests[i].json);
		utf8lite_render_clear(&render);
	}
}
END_TEST


START_TEST(test_escape_dquote)
{
	set_flags(0);
	ck_assert(!utf8lite_render_string(&render, "\""));
	ck_assert_str_eq(render.string, "\"");
	utf8lite_render_clear(&render);

	set_flags(UTF8LITE_ESCAPE_DQUOTE);
	ck_assert(!utf8lite_render_string(&render, "\""));
	ck_assert_str_eq(render.string, "\\\"");
	utf8lite_render_clear(&render);
}
END_TEST


START_TEST(test_escape_squote)
{
	set_flags(0);
	ck_assert(!utf8lite_render_string(&render, "\'"));
	ck_assert_str_eq(render.string, "\'");
	utf8lite_render_clear(&render);

	set_flags(UTF8LITE_ESCAPE_SQUOTE);
	ck_assert(!utf8lite_render_string(&render, "\'"));
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

	set_flags(0);
	ck_assert(!utf8lite_render_string(&render, "\\"));
	ck_assert_str_eq(render.string, "\\");
	utf8lite_render_clear(&render);

	for (i = 0; i < n; i++) {
		set_flags(flags[i]);
		ck_assert(!utf8lite_render_string(&render, "\\"));
		ck_assert_str_eq(render.string, "\\\\");
		utf8lite_render_clear(&render);
	}
}
END_TEST


START_TEST(test_escape_extended)
{
	const struct escape_test tests[] = {
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
		set_flags(0);
		ck_assert(!utf8lite_render_string(&render, tests[i].raw));
		ck_assert_str_eq(render.string, tests[i].raw);
		utf8lite_render_clear(&render);

		set_flags(flag | UTF8LITE_ENCODE_C);
		ck_assert(!utf8lite_render_string(&render, tests[i].raw));
		ck_assert_str_eq(render.string, tests[i].c);
		utf8lite_render_clear(&render);

		set_flags(flag | UTF8LITE_ENCODE_JSON);
		ck_assert(!utf8lite_render_string(&render, tests[i].raw));
		ck_assert_str_eq(render.string, tests[i].json);
		utf8lite_render_clear(&render);
	}
}
END_TEST


START_TEST(test_escape_utf8)
{
	const struct escape_test tests[] = {
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
		set_flags(0);
		ck_assert(!utf8lite_render_string(&render, tests[i].raw));
		ck_assert_str_eq(render.string, tests[i].raw);
		utf8lite_render_clear(&render);

		set_flags(flag | UTF8LITE_ENCODE_C);
		ck_assert(!utf8lite_render_string(&render, tests[i].raw));
		ck_assert_str_eq(render.string, tests[i].c);
		utf8lite_render_clear(&render);

		set_flags(flag | UTF8LITE_ENCODE_JSON);
		ck_assert(!utf8lite_render_string(&render, tests[i].raw));
		ck_assert_str_eq(render.string, tests[i].json);
		utf8lite_render_clear(&render);
	}
}
END_TEST


START_TEST(test_encode_char)
{
	set_flags(0);

	ck_assert(!utf8lite_render_char(&render, 'x'));
	ck_assert_str_eq(render.string, "x");

	ck_assert(!utf8lite_render_char(&render, 0x200B));
	ck_assert_str_eq(render.string, "x\xE2\x80\x8B");

	ck_assert(!utf8lite_render_char(&render, 'x'));
	ck_assert_str_eq(render.string, "x\xE2\x80\x8Bx");
	utf8lite_render_clear(&render);


	set_flags(UTF8LITE_ENCODE_RMDI);
	ck_assert(!utf8lite_render_char(&render, 'x'));
	ck_assert_str_eq(render.string, "x");

	ck_assert(!utf8lite_render_char(&render, 0x200B));
	ck_assert_str_eq(render.string, "x");

	ck_assert(!utf8lite_render_char(&render, 'x'));
	ck_assert_str_eq(render.string, "xx");
	utf8lite_render_clear(&render);
}
END_TEST


START_TEST(test_encode_rmdi)
{
	char buffer[32];
	uint8_t *end;
	int32_t codes[] = {
		0x00AD, 0x200B, 0x200C, 0x200D, 0x200E, 0x200F, 0x034F,
		0xFEFF, 0xE0001, 0xE0020, 0xE01EF
	};
	int flag = UTF8LITE_ENCODE_RMDI;
	int i, n = sizeof(codes) / sizeof(codes[0]);

	for (i = 0; i < n; i++) {
		end = (uint8_t *)buffer;
		utf8lite_encode_utf8(codes[i], &end);
		*end++ = '\0';

		set_flags(0);
		ck_assert(!utf8lite_render_string(&render, buffer));
		ck_assert_str_eq(render.string, buffer);
		utf8lite_render_clear(&render);

		set_flags(flag);
		ck_assert(!utf8lite_render_string(&render, buffer));
		ck_assert_str_eq(render.string, "");
		utf8lite_render_clear(&render);
	}
}
END_TEST


START_TEST(test_encode_emoji_plain)
{
	set_flags(0);

	ck_assert(!utf8lite_render_text(&render, JS("\\uD83D\\uDCF8")));
	ck_assert_str_eq(render.string, "\xf0\x9f\x93\xb8"); // U+1F4F8
	clear();

	ck_assert(!utf8lite_render_text(&render, JS("\\uD83D\\uDCF8\\u20E0")));
	ck_assert_str_eq(render.string, "\xf0\x9f\x93\xb8\xe2\x83\xa0");
	clear();

	set_flags(UTF8LITE_ESCAPE_UTF8);
	ck_assert(!utf8lite_render_text(&render, JS("\\uD83D\\uDCF8")));
	ck_assert_str_eq(render.string, "\\U0001f4f8");
	clear();

	ck_assert(!utf8lite_render_text(&render, JS("\\uD83D\\uDCF8\\u20E0")));
	ck_assert_str_eq(render.string, "\\U0001f4f8\\u20e0");
	clear();
}
END_TEST


START_TEST(test_encode_emoji_zwsp)
{
	set_flags(UTF8LITE_ENCODE_EMOJIZWSP);

	ck_assert(!utf8lite_render_text(&render, JS("\\u2614")));
	ck_assert_str_eq(render.string, "\xe2\x98\x94\xe2\x80\x8b");
	clear();

	set_flags(UTF8LITE_ENCODE_EMOJIZWSP | UTF8LITE_ESCAPE_UTF8);
	ck_assert(!utf8lite_render_text(&render, JS("\\u2614")));
	ck_assert_str_eq(render.string, "\\u2614");
	clear();

	set_flags(UTF8LITE_ENCODE_EMOJIZWSP | UTF8LITE_ESCAPE_EXTENDED);
	ck_assert(!utf8lite_render_text(&render, JS("\\u2614")));
	ck_assert_str_eq(render.string, "\xe2\x98\x94\xe2\x80\x8b");
	clear();
}
END_TEST


START_TEST(test_encode_emoji_extended_zwsp)
{
	set_flags(UTF8LITE_ENCODE_EMOJIZWSP);

	ck_assert(!utf8lite_render_text(&render, JS("\\uD83D\\uDCF8")));
	ck_assert_str_eq(render.string, "\xf0\x9f\x93\xb8\xe2\x80\x8b");
	clear();

	ck_assert(!utf8lite_render_text(&render, JS("\\uD83D\\uDCF8\\u20E0")));
	ck_assert_str_eq(render.string,
			"\xf0\x9f\x93\xb8\xe2\x83\xa0\xe2\x80\x8b");
	clear();

	set_flags(UTF8LITE_ENCODE_EMOJIZWSP | UTF8LITE_ESCAPE_UTF8);
	ck_assert(!utf8lite_render_text(&render, JS("\\uD83D\\uDCF8")));
	ck_assert_str_eq(render.string, "\\U0001f4f8");
	clear();

	ck_assert(!utf8lite_render_text(&render, JS("\\uD83D\\uDCF8\\u20E0")));
	ck_assert_str_eq(render.string, "\\U0001f4f8\\u20e0");
	clear();

	set_flags(UTF8LITE_ENCODE_EMOJIZWSP | UTF8LITE_ESCAPE_EXTENDED);
	ck_assert(!utf8lite_render_text(&render, JS("\\uD83D\\uDCF8")));
	ck_assert_str_eq(render.string, "\\U0001f4f8");
	clear();

	ck_assert(!utf8lite_render_text(&render, JS("\\uD83D\\uDCF8\\u20E0")));
	ck_assert_str_eq(render.string, "\\U0001f4f8\xe2\x83\xa0");
	clear();
}
END_TEST


START_TEST(test_encode_emoji_zwsp_rmdi)
{
	set_flags(UTF8LITE_ENCODE_EMOJIZWSP | UTF8LITE_ENCODE_RMDI);

	ck_assert(!utf8lite_render_text(&render, JS("\\u2614")));
	ck_assert_str_eq(render.string, "\xe2\x98\x94\xe2\x80\x8b");
	clear();

	set_flags(UTF8LITE_ENCODE_EMOJIZWSP | UTF8LITE_ESCAPE_UTF8);
	ck_assert(!utf8lite_render_text(&render, JS("\\u2614")));
	ck_assert_str_eq(render.string, "\\u2614");
	clear();

	set_flags(UTF8LITE_ENCODE_EMOJIZWSP | UTF8LITE_ESCAPE_EXTENDED);
	ck_assert(!utf8lite_render_text(&render, JS("\\u2614")));
	ck_assert_str_eq(render.string, "\xe2\x98\x94\xe2\x80\x8b");
	clear();
}
END_TEST


START_TEST(test_encode_emoji_zwj_sequence)
{
	set_flags(0);
	// \U0001F469\u200D\u2764\uFE0F\u200D\U0001F48B\u200D\U0001F469
        ck_assert(!utf8lite_render_text(&render, JS("\\ud83d\\udc69\\u200d\\u2764\\ufe0f\\u200d\\ud83d\\udc8b\\u200d\\ud83d\\udc69")));
	assert_text_eq(S(render.string), JS("\\ud83d\\udc69\\u200d\\u2764\\ufe0f\\u200d\\ud83d\\udc8b\\u200d\\ud83d\\udc69"));
	clear();

	set_flags(UTF8LITE_ENCODE_EMOJIZWSP | UTF8LITE_ENCODE_RMDI);
        ck_assert(!utf8lite_render_text(&render, JS("\\ud83d\\udc69\\u200d\\u2764\\ufe0f\\u200d\\ud83d\\udc8b\\u200d\\ud83d\\udc69")));

	assert_text_eq(S(render.string), JS("\\ud83d\\udc69\\u200d\\u2764\\ufe0f\\u200d\\ud83d\\udc8b\\u200d\\ud83d\\udc69\u200b"));
	clear();
}
END_TEST


START_TEST(test_byte_single)
{
	char byte;

	set_flags(UTF8LITE_ESCAPE_CONTROL);

	byte = 0x01;
	ck_assert(!utf8lite_render_raw(&render, &byte, 1));
	ck_assert_int_eq(render.string[0], byte);

	byte = (char)0xff;
	ck_assert(!utf8lite_render_raw(&render, &byte, 1));
	ck_assert_int_eq(render.string[0], 0x01);
	ck_assert_int_eq(render.string[1], (char)0xff);
}
END_TEST


START_TEST(test_byte_multiple)
{
	const char *bytes = "\x01\x02\x80\x00\x99\xfe";
	size_t i, n = 6;

	set_flags(UTF8LITE_ESCAPE_CONTROL);

	ck_assert(!utf8lite_render_raw(&render, bytes, sizeof(bytes)));
	for (i = 0; i < n; i++) {
		ck_assert_int_eq(render.string[i], bytes[i]);
	}
}
END_TEST


START_TEST(test_width_control_raw)
{
	set_flags(0);
	ck_assert_int_eq(width(S("\x01")), -1);
	ck_assert_int_eq(width(S("\a")), -1);
	ck_assert_int_eq(width(S("\n")), -1);
	ck_assert_int_eq(width(S("\r\n")), -1);
	ck_assert_int_eq(width(S("\x7F")), -1);
	ck_assert_int_eq(width(JS("\\u0e00")), -1);
	ck_assert_int_eq(width(JS("\\u2029")), -1);
	ck_assert_int_eq(width(JS("\\u2029")), -1);
	ck_assert_int_eq(width(JS("\\udbff\\udfff")), -1); // U+0010FFFF
}
END_TEST


START_TEST(test_width_control_esc)
{
	set_flags(UTF8LITE_ESCAPE_CONTROL);
	ck_assert_int_eq(width(S("\x01")), 6);
	ck_assert_int_eq(width(S("\a")), 2);
	ck_assert_int_eq(width(S("\n")), 2);
	ck_assert_int_eq(width(S("\r\n")), 4);
	ck_assert_int_eq(width(S("\x7F")), 6);
	ck_assert_int_eq(width(JS("\\u0e00")), 6);
	ck_assert_int_eq(width(JS("\\u2029")), 6);
	ck_assert_int_eq(width(JS("\\u2029")), 6);
	ck_assert_int_eq(width(JS("\\udbff\\udfff")), 10); // U+0010FFFF

	set_flags(UTF8LITE_ESCAPE_CONTROL | UTF8LITE_ENCODE_JSON);
	ck_assert_int_eq(width(S("\x01")), 6);
	ck_assert_int_eq(width(S("\a")), 6);
	ck_assert_int_eq(width(S("\n")), 2);
	ck_assert_int_eq(width(S("\r\n")), 4);
	ck_assert_int_eq(width(S("\x7F")), 6);
	ck_assert_int_eq(width(JS("\\u0e00")), 6);
	ck_assert_int_eq(width(JS("\\u2029")), 6);
	ck_assert_int_eq(width(JS("\\u2029")), 6);
	ck_assert_int_eq(width(JS("\\udbff\\udfff")), 12); // U+0010FFFF
}
END_TEST


START_TEST(test_width_ignorable_raw)
{
	char buffer[32];
	uint8_t *end;
	int32_t codes[] = {
		0x00AD, 0x200B, 0x200C, 0x200D, 0x200E, 0x200F, 0x034F,
		0xFEFF, 0xE0001, 0xE0020, 0xE01EF
	};
	int i, n = sizeof(codes) / sizeof(codes[0]);

	for (i = 0; i < n; i++) {
		end = (uint8_t *)buffer;
		utf8lite_encode_utf8(codes[i], &end);
		*end++ = '\0';

		set_flags(0);
		ck_assert_int_eq(width(S(buffer)), 0);

		set_flags(UTF8LITE_ESCAPE_UTF8);
		if (codes[i] <= 0xFFFF) {
			ck_assert_int_eq(width(S(buffer)), 6);
		} else {
			ck_assert_int_eq(width(S(buffer)), 10);
		}

		set_flags(UTF8LITE_ESCAPE_EXTENDED);
		if (codes[i] <= 0xFFFF) {
			ck_assert_int_eq(width(S(buffer)), 0);
		} else {
			ck_assert_int_eq(width(S(buffer)), 10);
		}
	}
}
END_TEST


START_TEST(test_width_ignorable_rm)
{
	char buffer[32];
	uint8_t *end;
	int32_t codes[] = {
		0x00AD, 0x200B, 0x200C, 0x200D, 0x200E, 0x200F, 0x034F,
		0xFEFF, 0xE0001, 0xE0020, 0xE01EF
	};
	int i, n = sizeof(codes) / sizeof(codes[0]);

	for (i = 0; i < n; i++) {
		end = (uint8_t *)buffer;
		utf8lite_encode_utf8(codes[i], &end);
		*end++ = '\0';

		set_flags(UTF8LITE_ENCODE_RMDI);
		ck_assert_int_eq(width(S(buffer)), 0);

		set_flags(UTF8LITE_ESCAPE_CONTROL);
		ck_assert_int_eq(width(S(buffer)), 0);
	}
}
END_TEST


START_TEST(test_width_dquote)
{
	set_flags(0);
	ck_assert_int_eq(width(S("\"")), 1);

	set_flags(UTF8LITE_ESCAPE_DQUOTE);
	ck_assert_int_eq(width(S("\"")), 2);
}
END_TEST


START_TEST(test_width_squote)
{
	set_flags(0);
	ck_assert_int_eq(width(S("'")), 1);

	set_flags(UTF8LITE_ESCAPE_SQUOTE);
	ck_assert_int_eq(width(S("'")), 2);
}
END_TEST


START_TEST(test_width_backslash)
{
	set_flags(0);
	ck_assert_int_eq(width(S("\\")), 1);
	ck_assert_int_eq(width(JS("\\\\")), 1);

	set_flags(UTF8LITE_ESCAPE_UTF8);
	ck_assert_int_eq(width(S("\\")), 2);
	ck_assert_int_eq(width(JS("\\\\")), 2);
}
END_TEST


START_TEST(test_width_ascii)
{
	set_flags(0);
	ck_assert_int_eq(width(S(" ")), 1);
	ck_assert_int_eq(width(S("~")), 1);

	set_flags(UTF8LITE_ESCAPE_UTF8);
	ck_assert_int_eq(width(S(" ")), 1);
	ck_assert_int_eq(width(S("~")), 1);

	set_flags(UTF8LITE_ESCAPE_EXTENDED);
	ck_assert_int_eq(width(S(" ")), 1);
	ck_assert_int_eq(width(S("~")), 1);
}
END_TEST


START_TEST(test_width_narrow)
{
	set_flags(0);
	ck_assert_int_eq(width(JS("\\u00a0")), 1);
	ck_assert_int_eq(width(JS("\\u00ff")), 1);
	ck_assert_int_eq(width(JS("\\u0100")), 1);
	ck_assert_int_eq(width(JS("\\u0101")), 1);
	ck_assert_int_eq(width(JS("\\u010b")), 1);
}
END_TEST


START_TEST(test_width_ambiguous)
{
	set_flags(0);
	ck_assert_int_eq(width(JS("\\u00a1")), 1);
	ck_assert_int_eq(width(JS("\\u016b")), 1);
	ck_assert_int_eq(width(JS("\\ufffd")), 1);
	ck_assert_int_eq(width(JS("\\ud83c\\udd00")), 1); // U+1F100

	set_flags(UTF8LITE_ENCODE_AMBIGWIDE);
	ck_assert_int_eq(width(JS("\\u00a1")), 2);
	ck_assert_int_eq(width(JS("\\u016b")), 2);
	ck_assert_int_eq(width(JS("\\ufffd")), 2);
	ck_assert_int_eq(width(JS("\\ud83c\\udd00")), 2); // U+1F100
}
END_TEST


START_TEST(test_width_wide)
{
	set_flags(0);

	ck_assert_int_eq(width(JS("\\uff01")), 2);
	ck_assert_int_eq(width(JS("\\uff02")), 2);
	ck_assert_int_eq(width(JS("\\uff03")), 2);
	ck_assert_int_eq(width(JS("\\uff04")), 2);
}
END_TEST


START_TEST(test_width_mark)
{
	set_flags(0);

	ck_assert_int_eq(width(JS("a\\u0300")), 1);
	ck_assert_int_eq(width(JS("a\\u030b")), 1);
	ck_assert_int_eq(width(JS("\\u0600a")), 1); // in theory, not practice

	// isolated mark
	// (not sure what the right behavior is here)
	ck_assert_int_eq(width(JS("\\u0300")), 0);
	ck_assert_int_eq(width(JS("\\u030b")), 0);
	ck_assert_int_eq(width(JS("\\u0600")), 0);
}
END_TEST


START_TEST(test_width_emoji)
{
	set_flags(0);

	ck_assert_int_eq(width(JS("\\uD83D\\uDCF8")), 2); // U+1F4F8
	ck_assert_int_eq(width(JS("\\uD83D\\uDCF8\\u20E0")), 2);
	// family of 4
	ck_assert_int_eq(width(JS("\\ud83d\\udc68"
				  "\\u200d\\ud83d\\udc69"
				  "\\u200d\\ud83d\\udc66"
				  "\\u200d\\ud83d\\udc66")), 2);
	// rainbow flag
	ck_assert_int_eq(width(JS("\\ud83c\\udff3\\ufe0f"
				  "\\u200d\\ud83c\\udf08")), 2);
}
END_TEST


START_TEST(test_width_emoji_escape)
{
	set_flags(UTF8LITE_ENCODE_C | UTF8LITE_ESCAPE_CONTROL
			| UTF8LITE_ESCAPE_UTF8);
	ck_assert_int_eq(width(JS("\\uD83D\\uDC87\\u200D\\u2642\\uFE0F")),
			 28);
}
END_TEST


Suite *render_suite(void)
{
        Suite *s;
        TCase *tc;

        s = suite_create("render");

	tc = tcase_create("format");
        tcase_add_checked_fixture(tc, setup_render, teardown_render);
        tcase_add_test(tc, test_format_spaces);
        tcase_add_test(tc, test_format_newlines);
        tcase_add_test(tc, test_format_newlines_custom);
        tcase_add_test(tc, test_format_indent);
        tcase_add_test(tc, test_format_indent_custom);
	tcase_add_test(tc, test_format_printf);
        suite_add_tcase(s, tc);

	tc = tcase_create("escape");
        tcase_add_checked_fixture(tc, setup_render, teardown_render);
        tcase_add_test(tc, test_escape_control);
        tcase_add_test(tc, test_escape_dquote);
        tcase_add_test(tc, test_escape_squote);
        tcase_add_test(tc, test_escape_backslash);
        tcase_add_test(tc, test_escape_extended);
        tcase_add_test(tc, test_escape_utf8);
        suite_add_tcase(s, tc);

	tc = tcase_create("encode");
        tcase_add_checked_fixture(tc, setup_render, teardown_render);
        tcase_add_test(tc, test_encode_char);
        tcase_add_test(tc, test_encode_rmdi);
        tcase_add_test(tc, test_encode_emoji_plain);
        tcase_add_test(tc, test_encode_emoji_zwsp);
        tcase_add_test(tc, test_encode_emoji_extended_zwsp);
        tcase_add_test(tc, test_encode_emoji_zwsp_rmdi);
	tcase_add_test(tc, test_encode_emoji_zwj_sequence);
        suite_add_tcase(s, tc);

	tc = tcase_create("bytes");
        tcase_add_test(tc, test_byte_single);
        tcase_add_test(tc, test_byte_multiple);
        suite_add_tcase(s, tc);

	tc = tcase_create("width");
        tcase_add_checked_fixture(tc, setup_render, teardown_render);
        tcase_add_test(tc, test_width_control_raw);
        tcase_add_test(tc, test_width_control_esc);
        tcase_add_test(tc, test_width_ignorable_raw);
        tcase_add_test(tc, test_width_ignorable_rm);
        tcase_add_test(tc, test_width_dquote);
        tcase_add_test(tc, test_width_squote);
        tcase_add_test(tc, test_width_backslash);
        tcase_add_test(tc, test_width_ascii);
        tcase_add_test(tc, test_width_narrow);
        tcase_add_test(tc, test_width_ambiguous);
        tcase_add_test(tc, test_width_wide);
        tcase_add_test(tc, test_width_mark);
        tcase_add_test(tc, test_width_emoji);
        tcase_add_test(tc, test_width_emoji_escape);
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
