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
#include <stdint.h>
#include <stdio.h>
#include "../src/utf8lite.h"
#include "wcwidth9/wcwidth9.h"
#include "testutil.h"

struct code_width {
	int32_t code;
	int width;
};

START_TEST(test_examples)
{
	struct code_width tests[] = {
		// Examples from https://github.com/patperry/r-utf8/issues/9
		{.code = 0x2139, .width = UTF8LITE_CHARWIDTH_NARROW},
		{.code = 0x2600, .width = UTF8LITE_CHARWIDTH_NARROW},
		{.code = 0x2728, .width = UTF8LITE_CHARWIDTH_EMOJI}
	};
	struct code_width *t;
	int i, n, ok, prop, prop0, nfail;

	n = (int)(sizeof(tests) / sizeof(tests[0]));
	nfail = 0;
	for (i = 0; i < n; i++) {
		t = &tests[i];
		prop0 = t->width;
		prop = utf8lite_charwidth(t->code);
		ok = (prop == prop0);

		if (!ok) {
			nfail++;
			printf("U+%04X expected: %d got: %d\n", t->code,
				prop0, prop);
		}
	}

	ck_assert(nfail == 0);
}
END_TEST

/*
 * This check is kind of meaningless. wcwidth9 has Unicode 9.0.0, gives
 * different behavior for lots of characters.
 */
START_TEST(test_wcwidth9)
{
	int prop, prop0, ok, nfail;
	int32_t code;

	nfail = 0;
	for (code = 0; code <= UTF8LITE_CODE_MAX; code++) {
		prop0 = (code < 0x10FFFE) ? wcwidth9(code) : -3;
		prop = utf8lite_charwidth(code);

		if (code == 0x1F93B || code == 0x1F946) {
			// These characters changed from East Asian Wide
			// to Narrow in Unicode 13.0
			ok = prop == UTF8LITE_CHARWIDTH_NARROW;
			goto Check;
		} else if (0x1F1E6 <= code && code <= 0x1F1FF) {
			// regional indicators
			ok = prop == UTF8LITE_CHARWIDTH_NARROW;
			goto Check;
		}

		switch (prop) {
		case UTF8LITE_CHARWIDTH_NONE:
			ok = prop0 == -1 || prop0 == -3 || prop0 == 1;
			break;

		case UTF8LITE_CHARWIDTH_IGNORABLE:
			ok = prop0 == -1 || prop0 >= 1;
			break;

		case UTF8LITE_CHARWIDTH_MARK:
			ok = prop0 == -1 || prop0 == 1;
			break;

		case UTF8LITE_CHARWIDTH_NARROW:
			ok = prop0 == 1 || prop0 == -1 || code > 0xFFFF;
			break;

		case UTF8LITE_CHARWIDTH_AMBIGUOUS:
			ok = prop0 == -2;
			break;

		case UTF8LITE_CHARWIDTH_WIDE:
			ok = prop0 == 2 || prop0 == -1;
			break;

		case UTF8LITE_CHARWIDTH_EMOJI:
			ok = prop0 == 2 || prop0 == 1 || prop0 == -1 || prop0 == -2;
			break;

		default:
			ok = 0;
			break;
		}
Check:
		if (!ok) {
			nfail++;
			printf("U+%04X wcwidth9: %d utf8lite: %d\n", code, prop0, prop);
		}
	}

	ck_assert(nfail == 0);
}
END_TEST


Suite *charwidth_suite(void)
{
        Suite *s;
        TCase *tc;

        s = suite_create("charwidth");
	tc = tcase_create("core");
        tcase_add_test(tc, test_examples);
        suite_add_tcase(s, tc);

	tc = tcase_create("wcwidth9");
        tcase_add_test(tc, test_wcwidth9);
        suite_add_tcase(s, tc);

	return s;
}


int main(void)
{
        int number_failed;
        Suite *s;
        SRunner *sr;

        s = charwidth_suite();
        sr = srunner_create(s);

        srunner_run_all(sr, CK_NORMAL);
        number_failed = srunner_ntests_failed(sr);
        srunner_free(sr);

        return (number_failed == 0) ? EXIT_SUCCESS : EXIT_FAILURE;
}
