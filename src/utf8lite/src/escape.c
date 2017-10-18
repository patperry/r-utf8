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

#include <ctype.h>
#include "utf8lite.h"

/* http://stackoverflow.com/a/11986885 */
#define hextoi(ch) ((ch > '9') ? (ch &~ 0x20) - 'A' + 10 : (ch - '0'))

int utf8lite_scan_escape(const uint8_t **bufptr, const uint8_t *end,
			 struct utf8lite_message *msg)
{
	const uint8_t *input = *bufptr;
	const uint8_t *ptr = input;
	uint_fast8_t ch;
	int err;
	
	if (ptr == end) {
		goto error_incomplete;
	}

	ch = *ptr++;

	switch (ch) {
	case '"':
	case '\\':
	case '/':
	case 'b':
	case 'f':
	case 'n':
	case 'r':
	case 't':
		break;
	case 'u':
		if ((err = utf8lite_scan_uescape(&ptr, end, msg))) {
			goto out;
		}
		break;
	default:
		goto error_inval;
	}

	err = 0;
	goto out;

error_incomplete:
	err = UTF8LITE_ERROR_INVAL;
	utf8lite_message_set(msg, "incomplete escape code (\\)");
	goto out;

error_inval:
	err = UTF8LITE_ERROR_INVAL;
	utf8lite_message_set(msg, "invalid escape code (\\%c)", ch);
	goto out;

out:
	*bufptr = ptr;
	return err;
}


int utf8lite_scan_uescape(const uint8_t **bufptr, const uint8_t *end,
			  struct utf8lite_message *msg)
{
	const uint8_t *input = *bufptr;
	const uint8_t *ptr = input;
	int32_t code, low;
	uint_fast8_t ch;
	unsigned i;
	int err;

	if (ptr + 4 > end) {
		goto error_inval_incomplete;
	}

	code = 0;
	for (i = 0; i < 4; i++) {
		ch = *ptr++;
		if (!isxdigit(ch)) {
			goto error_inval_hex;
		}
		code = (code << 4) + hextoi(ch);
	}

	if (UTF8LITE_IS_UTF16_HIGH(code)) {
		if (ptr + 6 > end || ptr[0] != '\\' || ptr[1] != 'u') {
			goto error_inval_nolow;
		}
		ptr += 2;
		input = ptr;

		low = 0;
		for (i = 0; i < 4; i++) {
			ch = *ptr++;
			if (!isxdigit(ch)) {
				goto error_inval_hex;
			}
			low = (low << 4) + hextoi(ch);
		}
		if (!UTF8LITE_IS_UTF16_LOW(low)) {
			ptr -= 6;
			goto error_inval_low;
		}
	} else if (UTF8LITE_IS_UTF16_LOW(code)) {
		goto error_inval_nohigh;
	}

	err = 0;
	goto out;

error_inval_incomplete:
	err = UTF8LITE_ERROR_INVAL;
	utf8lite_message_set(msg, "incomplete escape code (\\u%.*s)",
			     (int)(end - input), input);
	goto out;

error_inval_hex:
	err = UTF8LITE_ERROR_INVAL;
	utf8lite_message_set(msg, "invalid hex value in escape code (\\u%.*s)",
			     4, input);
	goto out;

error_inval_nolow:
	err = UTF8LITE_ERROR_INVAL;
	utf8lite_message_set(msg, "missing UTF-16 low surrogate"
			     " after high surrogate escape code (\\u%.*s)",
			     4, input);
	goto out;

error_inval_low:
	err = UTF8LITE_ERROR_INVAL;
	utf8lite_message_set(msg, "invalid UTF-16 low surrogate (\\u%.*s)"
			     " after high surrogate escape code (\\u%.*s)",
			     4, input, 4, input - 6);
	goto out;

error_inval_nohigh:
	err = UTF8LITE_ERROR_INVAL;
	utf8lite_message_set(msg, "missing UTF-16 high surrogate"
			     " before low surrogate escape code (\\u%.*s)",
			     4, input);
	goto out;

out:
	*bufptr = ptr;
	return err;
}


void utf8lite_decode_uescape(const uint8_t **inputptr, int32_t *codeptr)
{
	const uint8_t *ptr = *inputptr;
	int32_t code;
	uint_fast16_t low;
	uint_fast8_t ch;
	unsigned i;

	code = 0;
	for (i = 0; i < 4; i++) {
		ch = *ptr++;
		code = (code << 4) + hextoi(ch);
	}

	if (UTF8LITE_IS_UTF16_HIGH(code)) {
		// skip over \u
		ptr += 2;

		low = 0;
		for (i = 0; i < 4; i++) {
			ch = *ptr++;
			low = (uint_fast16_t)(low << 4) + hextoi(ch);
		}

		code = UTF8LITE_DECODE_UTF16_PAIR(code, low);
	}

	*codeptr = code;
	*inputptr = ptr;
}


void utf8lite_decode_escape(const uint8_t **inputptr, int32_t *codeptr)
{
	const uint8_t *ptr = *inputptr;
	int32_t code;

	code = *ptr++;

	switch (code) {
	case 'b':
		code = '\b';
		break;
	case 'f':
		code = '\f';
		break;
	case 'n':
		code = '\n';
		break;
	case 'r':
		code = '\r';
		break;
	case 't':
		code = '\t';
		break;
	case 'u':
		*inputptr = ptr;
		utf8lite_decode_uescape(inputptr, codeptr);
		return;
	default:
		break;
	}

	*inputptr = ptr;
	*codeptr = code;
}
