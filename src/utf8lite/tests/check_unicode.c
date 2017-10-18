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

#include <inttypes.h>
#include <stdio.h>
#include <stdlib.h>
#include <check.h>
#include "../src/utf8lite.h"
#include "testutil.h"


#define NORMALIZATION_TEST "data/ucd/NormalizationTest.txt"

// Unicode Normalization Test
// http://www.unicode.org/Public/UCD/latest/ucd/NormalizationTest.txt
struct normalization_test {
	char comment[1024];
	int line;
	int part;

	int32_t source[128];
	int source_len;

	int32_t nfc[128];
	int nfc_len;

	int32_t nfd[128];
	int nfd_len;

	int32_t nfkc[128];
	int nfkc_len;

	int32_t nfkd[128];
	int nfkd_len;
};

struct normalization_test normalization_tests[32768];
int num_normalization_test;

void write_normalization_test(FILE *stream,
			      const struct normalization_test *test)
{
	int i;

	for (i = 0; i < test->source_len; i++) {
		if (i > 0) {
			fprintf(stream, " ");
		}
		fprintf(stream, "%04X", test->source[i]);
	}
	fprintf(stream, ";");

	for (i = 0; i < test->nfc_len; i++) {
		if (i > 0) {
			fprintf(stream, " ");
		}
		fprintf(stream, "%04X", test->nfc[i]);
	}
	fprintf(stream, ";");

	for (i = 0; i < test->nfd_len; i++) {
		if (i > 0) {
			fprintf(stream, " ");
		}
		fprintf(stream, "%04X", test->nfd[i]);
	}
	fprintf(stream, ";");

	for (i = 0; i < test->nfkc_len; i++) {
		if (i > 0) {
			fprintf(stream, " ");
		}
		fprintf(stream, "%04X", test->nfkc[i]);
	}
	fprintf(stream, ";");

	for (i = 0; i < test->nfkd_len; i++) {
		if (i > 0) {
			fprintf(stream, " ");
		}
		fprintf(stream, "%04X", test->nfkd[i]);
	}
	fprintf(stream, ";");

	fprintf(stream, "%s\n", test->comment);
}


void setup_normalization(void)
{
	struct normalization_test *test;
	FILE *file;
	int part;
	int field, line, ntest;
	uint32_t code;
	int32_t *dst;
	int *lenp;
	char *comment;
	int ch;

	file = fopen(NORMALIZATION_TEST, "r");
	if (!file) {
		file = fopen("../"NORMALIZATION_TEST, "r");
	}
	ck_assert_msg(file != NULL, "file '"NORMALIZATION_TEST"' not found");

	ntest = 0;
	test = &normalization_tests[ntest];
	test->comment[0] = '\0';
	test->source_len = 0;
	test->nfc_len = 0;
	test->nfd_len = 0;
	test->nfkc_len = 0;
	test->nfkd_len = 0;
	dst = test->source;
	lenp = &test->source_len;

	part = -1;
	line = 1;
	field = 0;

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
			test->line = line;
			test->part = part;
			if (test->source_len > 0) {
				ntest++;
				test = &normalization_tests[ntest];
				test->comment[0] = '\0';
				test->source_len = 0;
				test->nfc_len = 0;
				test->nfd_len = 0;
				test->nfkc_len = 0;
				test->nfkd_len = 0;
				dst = test->source;
				lenp = &test->source_len;
			}
			line++;
			field = 0;
			break;
		case ';':
			field++;
			switch (field) {
			case 1:
				dst = test->nfc;
				lenp = &test->nfc_len;
				break;
			case 2:
				dst = test->nfd;
				lenp = &test->nfd_len;
				break;
			case 3:
				dst = test->nfkc;
				lenp = &test->nfkc_len;
				break;
			case 4:
				dst = test->nfkd;
				lenp = &test->nfkd_len;
				break;
			case 5:
				break;
			default:
				fprintf(stderr, "too many fields\n");
				goto inval;
			}
			break;
		case ' ':
			break;
		case '@':
			ck_assert(fscanf(file, "Part%d", &part));
			break;
		default:
			ungetc(ch, file);
			ck_assert(fscanf(file, "%"SCNx32, &code));
			*dst++ = (int32_t)code;
			*lenp = *lenp + 1;
			break;
		}
	}
eof:
	num_normalization_test = ntest;
	return;
inval:
	fprintf(stderr, "invalid character on line %d\n", line);

	fclose(file);
}


void teardown_normalization(void)
{
}


int is_utf8(const char *str, size_t len)
{
	struct utf8lite_message msg;
	const uint8_t *ptr = (const uint8_t *)str;
	const uint8_t *end = ptr + len;
        int err;

        while (ptr < end) {
                if ((err = utf8lite_scan_utf8(&ptr, end, &msg))) {
			return 0;
                }
        }

        return 1;
}

START_TEST(test_accept_valid_1byte_utf8)
{
	ck_assert(is_utf8("\x00", 1));
	ck_assert(is_utf8("\x01", 1));
	ck_assert(is_utf8("\x7E", 1));
	ck_assert(is_utf8("\x7F", 1));

}
END_TEST

START_TEST(test_reject_invalid_1byte_utf8)
{
	ck_assert(!is_utf8("\x80", 1));
	ck_assert(!is_utf8("\xBF", 1));
	ck_assert(!is_utf8("\xC0", 1));
	ck_assert(!is_utf8("\xE0", 1));
	ck_assert(!is_utf8("\xF0", 1));
	ck_assert(!is_utf8("\xF8", 1));
	ck_assert(!is_utf8("\xFC", 1));
	ck_assert(!is_utf8("\xFE", 1));
	ck_assert(!is_utf8("\xFF", 1));
}
END_TEST

START_TEST(test_accept_valid_2byte_utf8)
{
	ck_assert(is_utf8("\xC2\x80", 2));
	ck_assert(is_utf8("\xC2\x8F", 2));
	ck_assert(is_utf8("\xDF\x80", 2));
	ck_assert(is_utf8("\xDF\x8F", 2));
}
END_TEST

START_TEST(test_reject_invalid_2byte_utf8)
{
	// invalid first byte, valid second byte
	ck_assert(!is_utf8("\x80\x80", 2));
	ck_assert(!is_utf8("\xC1\x8F", 2));
	ck_assert(!is_utf8("\xF5\x80", 2));
	ck_assert(!is_utf8("\xFF\x80", 2));
	ck_assert(!is_utf8("\xFF\xFF", 2));

	// valid first byte, invalid second byte
	ck_assert(!is_utf8("\xC2\x00", 2));
	ck_assert(!is_utf8("\xC2\x7F", 2));
	ck_assert(!is_utf8("\xDF\x00", 2));
	ck_assert(!is_utf8("\xDF\x7F", 2));

	// too short
	ck_assert(!is_utf8("\xE0\x80", 2));
	ck_assert(!is_utf8("\xE0\xA0", 2));
	ck_assert(!is_utf8("\xE1\x80", 2));
	ck_assert(!is_utf8("\xEC\x80", 2));
	ck_assert(!is_utf8("\xEE\x80", 2));
	ck_assert(!is_utf8("\xED\x80", 2));
	ck_assert(!is_utf8("\xEF\x80", 2));
	ck_assert(!is_utf8("\xF0\x80", 2));
	ck_assert(!is_utf8("\xF0\x90", 2));
	ck_assert(!is_utf8("\xF1\x80", 2));
	ck_assert(!is_utf8("\xF4\x80", 2));
}
END_TEST

START_TEST(test_accept_valid_3byte_utf8)
{
	ck_assert(is_utf8("\xE0\xA0\x80", 3));
	ck_assert(is_utf8("\xE0\xA0\xBF", 3));
	ck_assert(is_utf8("\xE0\xBF\x80", 3));
	ck_assert(is_utf8("\xE0\xBF\xBF", 3));

	ck_assert(is_utf8("\xE1\x80\x80", 3));
	ck_assert(is_utf8("\xE1\x80\xBF", 3));
	ck_assert(is_utf8("\xE1\xBF\x80", 3));
	ck_assert(is_utf8("\xE1\xBF\xBF", 3));
	ck_assert(is_utf8("\xEC\x80\x80", 3));
	ck_assert(is_utf8("\xEC\x80\xBF", 3));
	ck_assert(is_utf8("\xEC\xBF\x80", 3));
	ck_assert(is_utf8("\xEC\xBF\xBF", 3));

	ck_assert(is_utf8("\xED\x80\x80", 3));
	ck_assert(is_utf8("\xED\x80\xBF", 3));
	ck_assert(is_utf8("\xED\x9F\x80", 3));
	ck_assert(is_utf8("\xED\x9F\xBF", 3));
}
END_TEST

START_TEST(test_reject_invalid_3byte_utf8)
{
	ck_assert(!is_utf8("\xE0\x80\x80", 3));
	ck_assert(!is_utf8("\xE0\x80\xBF", 3));
	ck_assert(!is_utf8("\xE0\x9F\x80", 3));
	ck_assert(!is_utf8("\xE0\x9F\xBF", 3));

	ck_assert(!is_utf8("\xED\xA0\x80", 3));
	ck_assert(!is_utf8("\xED\xA0\xBF", 3));
	ck_assert(!is_utf8("\xED\xBF\x80", 3));
	ck_assert(!is_utf8("\xED\xBF\xBF", 3));
}
END_TEST

START_TEST(test_accept_valid_4byte_utf8)
{
	ck_assert(is_utf8("\xF0\x90\x80\x80", 4));
	ck_assert(is_utf8("\xF0\x90\x80\xBF", 4));
	ck_assert(is_utf8("\xF0\x90\xBF\x80", 4));
	ck_assert(is_utf8("\xF0\x90\xBF\xBF", 4));
	ck_assert(is_utf8("\xF0\xBF\x80\x80", 4));
	ck_assert(is_utf8("\xF0\xBF\x80\xBF", 4));
	ck_assert(is_utf8("\xF0\xBF\xBF\x80", 4));
	ck_assert(is_utf8("\xF0\xBF\xBF\xBF", 4));

	ck_assert(is_utf8("\xF1\x80\x80\x80", 4));
	ck_assert(is_utf8("\xF1\x80\x80\xBF", 4));
	ck_assert(is_utf8("\xF1\x80\xBF\x80", 4));
	ck_assert(is_utf8("\xF1\x80\xBF\xBF", 4));
	ck_assert(is_utf8("\xF1\xBF\x80\x80", 4));
	ck_assert(is_utf8("\xF1\xBF\x80\xBF", 4));
	ck_assert(is_utf8("\xF1\xBF\xBF\x80", 4));
	ck_assert(is_utf8("\xF1\xBF\xBF\xBF", 4));

	ck_assert(is_utf8("\xF3\x80\x80\x80", 4));
	ck_assert(is_utf8("\xF3\x80\x80\xBF", 4));
	ck_assert(is_utf8("\xF3\x80\xBF\x80", 4));
	ck_assert(is_utf8("\xF3\x80\xBF\xBF", 4));
	ck_assert(is_utf8("\xF3\xBF\x80\x80", 4));
	ck_assert(is_utf8("\xF3\xBF\x80\xBF", 4));
	ck_assert(is_utf8("\xF3\xBF\xBF\x80", 4));
	ck_assert(is_utf8("\xF3\xBF\xBF\xBF", 4));

	ck_assert(is_utf8("\xF4\x80\x80\x80", 4));
	ck_assert(is_utf8("\xF4\x80\x80\xBF", 4));
	ck_assert(is_utf8("\xF4\x80\xBF\x80", 4));
	ck_assert(is_utf8("\xF4\x80\xBF\xBF", 4));
	ck_assert(is_utf8("\xF4\x8F\x80\x80", 4));
	ck_assert(is_utf8("\xF4\x8F\x80\xBF", 4));
	ck_assert(is_utf8("\xF4\x8F\xBF\x80", 4));
	ck_assert(is_utf8("\xF4\x8F\xBF\xBF", 4));
}
END_TEST

START_TEST(test_reject_invalid_4byte_utf8)
{
	ck_assert(!is_utf8("\xF0\x80\x80\x80", 4));
	ck_assert(!is_utf8("\xF0\x8F\x80\x80", 4));
	ck_assert(!is_utf8("\xF4\x90\x80\x80", 4));
	ck_assert(!is_utf8("\xF4\xBF\x80\x80", 4));

	ck_assert(!is_utf8("\xF5\x80\x80\x80", 4));
	ck_assert(!is_utf8("\xFF\x80\x80\x80", 4));
}
END_TEST


static void roundtrip(int32_t code)
{
	uint8_t buf[6];
	uint8_t *ptr;
	int32_t decode;

	ptr = buf;
	utf8lite_encode_utf8(code, &ptr);

	ck_assert_int_eq(ptr - buf, UTF8LITE_UTF8_ENCODE_LEN(code));
	ck_assert(is_utf8((const char *)buf, UTF8LITE_UTF8_ENCODE_LEN(code)));

	ptr = buf;
	utf8lite_decode_utf8((const uint8_t **)&ptr, &decode);
	ck_assert_int_eq(ptr - buf, UTF8LITE_UTF8_ENCODE_LEN(code));
	ck_assert_int_eq(code, decode);
}


START_TEST(test_encode_decode_utf8)
{
	int32_t code;

	// U+0000..U+FFFF
	for (code = 0; code <= 0xFFFF; code += 0xFF) {
		if (!UTF8LITE_IS_UNICODE(code)) {
			continue;
		}
		roundtrip(code);
	}

	// U+10000..U+3FFFF
	roundtrip(0x10000);
	roundtrip(0x10001);
	roundtrip(0x3FFFE);
	roundtrip(0x3FFFF);

	// U+40000..U+FFFFF
	roundtrip(0x40000);
	roundtrip(0x40001);
	roundtrip(0xFFFFE);
	roundtrip(0xFFFFF);

	// U+100000..U+10FFFF
	roundtrip(0x100000);
	roundtrip(0x100001);
	roundtrip(0x10FFFE);
	roundtrip(0x10FFFF);
}
END_TEST


static void reverse_roundtrip(int32_t code)
{
	uint8_t buf[6];
	uint8_t *end = buf + 6;
	uint8_t *start, *ptr;
	int32_t decode;

	ptr = end;
	utf8lite_rencode_utf8(code, &ptr);

	ck_assert_int_eq(end - ptr, UTF8LITE_UTF8_ENCODE_LEN(code));
	ck_assert(is_utf8((const char *)ptr, UTF8LITE_UTF8_ENCODE_LEN(code)));

	start = ptr;
	utf8lite_decode_utf8((const uint8_t **)&ptr, &decode);
	ck_assert_int_eq(ptr - start, UTF8LITE_UTF8_ENCODE_LEN(code));
	ck_assert_int_eq(code, decode);
}


START_TEST(test_rencode_decode_utf8)
{
	int32_t code;

	// U+0000..U+FFFF
	for (code = 0; code <= 0xFFFF; code += 0xFF) {
		if (!UTF8LITE_IS_UNICODE(code)) {
			continue;
		}
		reverse_roundtrip(code);
	}

	// U+10000..U+3FFFF
	reverse_roundtrip(0x10000);
	reverse_roundtrip(0x10001);
	reverse_roundtrip(0x3FFFE);
	reverse_roundtrip(0x3FFFF);

	// U+40000..U+FFFFF
	reverse_roundtrip(0x40000);
	reverse_roundtrip(0x40001);
	reverse_roundtrip(0xFFFFE);
	reverse_roundtrip(0xFFFFF);

	// U+100000..U+10FFFF
	reverse_roundtrip(0x100000);
	reverse_roundtrip(0x100001);
	reverse_roundtrip(0x10FFFE);
	reverse_roundtrip(0x10FFFF);
}
END_TEST


START_TEST(test_normalize_nfc_nfd)
{
	int i, j, n = num_normalization_test;
	size_t len;
	struct normalization_test *test;
	int32_t buf[128];
	int32_t *dst;
	int32_t code;

	for (i = 0; i < n; i++) {
		test = &normalization_tests[i];
		dst = buf;
		for (j = 0; j < test->source_len; j++) {
			code = test->source[j];
			utf8lite_map(0, code, &dst);
		}

		len = (size_t)(dst - buf);
		ck_assert_uint_eq(len, (size_t)test->nfd_len);

		utf8lite_order(buf, len);
		for (j = 0; j < test->nfd_len; j++) {
			if (buf[j] != test->nfd[j]) {
				write_normalization_test(stderr, test);
			}
			ck_assert_int_eq(buf[j], test->nfd[j]);
		}

		utf8lite_compose(buf, &len);
		ck_assert_uint_eq(len, (size_t)test->nfc_len);

		for (j = 0; j < test->nfc_len; j++) {
			if (buf[j] != test->nfc[j]) {
				write_normalization_test(stderr, test);
			}
			ck_assert_int_eq(buf[j], test->nfc[j]);
		}
	}
}
END_TEST


START_TEST(test_normalize_nfkc_nfkd)
{
	int i, j, n = num_normalization_test;
	size_t len;
	struct normalization_test *test;
	int32_t buf[128];
	int32_t *dst;
	int32_t code;

	for (i = 0; i < n; i++) {
		test = &normalization_tests[i];
		dst = buf;
		for (j = 0; j < test->source_len; j++) {
			code = test->source[j];
			utf8lite_map(UTF8LITE_DECOMP_ALL, code, &dst);
		}

		len = (size_t)(dst - buf);
		ck_assert_uint_eq(len, (size_t)test->nfkd_len);

		utf8lite_order(buf, len);

		for (j = 0; j < test->nfkd_len; j++) {
			if (buf[j] != test->nfkd[j]) {
				write_normalization_test(stderr, test);
			}
			ck_assert_int_eq(buf[j], test->nfkd[j]);
		}

		utf8lite_compose(buf, &len);
		ck_assert_uint_eq(len, (size_t)test->nfkc_len);

		for (j = 0; j < test->nfkc_len; j++) {
			if (buf[j] != test->nfkc[j]) {
				write_normalization_test(stderr, test);
			}
			ck_assert_int_eq(buf[j], test->nfkc[j]);
		}

	}
}
END_TEST


START_TEST(test_whitespace)
{
	ck_assert(!utf8lite_isspace(0x08));
	ck_assert(utf8lite_isspace('\t'));
	ck_assert(utf8lite_isspace('\n'));
	ck_assert(utf8lite_isspace('\v'));
	ck_assert(utf8lite_isspace('\f'));
	ck_assert(utf8lite_isspace('\r'));
	ck_assert(!utf8lite_isspace(0x0E));
	ck_assert(utf8lite_isspace(' '));

	ck_assert(utf8lite_isspace(0x85));
	ck_assert(!utf8lite_isspace(0x86));
	ck_assert(utf8lite_isspace(0xA0));
	ck_assert(utf8lite_isspace(0xA0));

	ck_assert(!utf8lite_isspace(0x1FFF));
	ck_assert(utf8lite_isspace(0x2000));
	ck_assert(utf8lite_isspace(0x2001));
	ck_assert(utf8lite_isspace(0x200A));
	ck_assert(!utf8lite_isspace(0x200B));

	ck_assert(utf8lite_isspace(0x2028));
	ck_assert(utf8lite_isspace(0x2029));
	ck_assert(!utf8lite_isspace(0x202A));
	ck_assert(utf8lite_isspace(0x3000));
	ck_assert(!utf8lite_isspace(0x3001));
}
END_TEST


START_TEST(test_ignorable)
{
	ck_assert(!utf8lite_isignorable(0x00));
	ck_assert(!utf8lite_isignorable(0x7F));

	ck_assert(utf8lite_isignorable(0x00AD));
	ck_assert(utf8lite_isignorable(0x034F));
	ck_assert(utf8lite_isignorable(0x061C));
	ck_assert(utf8lite_isignorable(0x115F));
	ck_assert(utf8lite_isignorable(0x1160));
	ck_assert(utf8lite_isignorable(0xFE00));
	ck_assert(utf8lite_isignorable(0x1BCA0));

	ck_assert(!utf8lite_isignorable(0xDFFFF));
	ck_assert(utf8lite_isignorable(0xE0000));
	ck_assert(utf8lite_isignorable(0xE0FFF));
	ck_assert(!utf8lite_isignorable(0xE1000));
}
END_TEST


Suite *unicode_suite(void)
{
	Suite *s;
	TCase *tc;

	s = suite_create("unicode");

	tc = tcase_create("utf8 validation");
	tcase_add_test(tc, test_accept_valid_1byte_utf8);
	tcase_add_test(tc, test_reject_invalid_1byte_utf8);
	tcase_add_test(tc, test_accept_valid_2byte_utf8);
	tcase_add_test(tc, test_reject_invalid_2byte_utf8);
	tcase_add_test(tc, test_accept_valid_3byte_utf8);
	tcase_add_test(tc, test_reject_invalid_3byte_utf8);
	tcase_add_test(tc, test_accept_valid_4byte_utf8);
	tcase_add_test(tc, test_reject_invalid_4byte_utf8);
	suite_add_tcase(s, tc);

	tc = tcase_create("utf8 encoding and decoding");
	tcase_add_test(tc, test_encode_decode_utf8);
	tcase_add_test(tc, test_rencode_decode_utf8);
	suite_add_tcase(s, tc);

	tc = tcase_create("utf32 normalization");
	tcase_add_checked_fixture(tc, setup_normalization,
				  teardown_normalization);
	tcase_add_test(tc, test_normalize_nfc_nfd);
	tcase_add_test(tc, test_normalize_nfkc_nfkd);
	suite_add_tcase(s, tc);

	tc = tcase_create("utf8 character properties");
	tcase_add_test(tc, test_whitespace);
	tcase_add_test(tc, test_ignorable);
	suite_add_tcase(s, tc);

	return s;
}

int main(void)
{
	int number_failed;
	Suite *s;
	SRunner *sr;

	s = unicode_suite();
	sr = srunner_create(s);

	srunner_run_all(sr, CK_NORMAL);
	number_failed = srunner_ntests_failed(sr);
	srunner_free(sr);
	return (number_failed == 0) ? EXIT_SUCCESS : EXIT_FAILURE;
}
