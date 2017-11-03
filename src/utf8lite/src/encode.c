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

#include <assert.h>
#include "utf8lite.h"

/*
  Source:
   http://www.unicode.org/versions/Unicode7.0.0/UnicodeStandard-7.0.pdf
   page 124, 3.9 "Unicode Encoding Forms", "UTF-8"

  Table 3-7. Well-Formed UTF-8 Byte Sequences
  -----------------------------------------------------------------------------
  |  Code Points        | First Byte | Second Byte | Third Byte | Fourth Byte |
  |  U+0000..U+007F     |     00..7F |             |            |             |
  |  U+0080..U+07FF     |     C2..DF |      80..BF |            |             |
  |  U+0800..U+0FFF     |         E0 |      A0..BF |     80..BF |             |
  |  U+1000..U+CFFF     |     E1..EC |      80..BF |     80..BF |             |
  |  U+D000..U+D7FF     |         ED |      80..9F |     80..BF |             |
  |  U+E000..U+FFFF     |     EE..EF |      80..BF |     80..BF |             |
  |  U+10000..U+3FFFF   |         F0 |      90..BF |     80..BF |      80..BF |
  |  U+40000..U+FFFFF   |     F1..F3 |      80..BF |     80..BF |      80..BF |
  |  U+100000..U+10FFFF |         F4 |      80..8F |     80..BF |      80..BF |
  -----------------------------------------------------------------------------

  (table taken from https://github.com/JulienPalard/is_utf8 )
*/


int utf8lite_scan_utf8(const uint8_t **bufptr, const uint8_t *end,
		       struct utf8lite_message *msg)
{
	const uint8_t *ptr = *bufptr;
	uint_fast8_t ch, ch1;
	unsigned nc;
	int err;

	assert(ptr < end);

	/* First byte
	 * ----------
	 *
	 * 1-byte sequence:
	 * 00: 0000 0000
	 * 7F: 0111 1111
	 * (ch1 & 0x80 == 0)
	 *
	 * Invalid:
	 * 80: 1000 0000
	 * BF: 1011 1111
	 * C0: 1100 0000
	 * C1: 1100 0001
	 * (ch & 0xF0 == 0x80 || ch == 0xC0 || ch == 0xC1)
	 *
	 * 2-byte sequence:
	 * C2: 1100 0010
	 * DF: 1101 1111
	 * (ch & 0xE0 == 0xC0 && ch > 0xC1)
	 *
	 * 3-byte sequence
	 * E0: 1110 0000
	 * EF: 1110 1111
	 * (ch & 0xF0 == E0)
	 *
	 * 4-byte sequence:
	 * F0: 1111 0000
	 * F4: 1111 0100
	 * (ch & 0xFC == 0xF0 || ch == 0xF4)
	 */

	ch1 = *ptr++;

	if ((ch1 & 0x80) == 0) {
		goto success;
	} else if ((ch1 & 0xC0) == 0x80) {
		goto inval_lead;
	} else if ((ch1 & 0xE0) == 0xC0) {
		if (ch1 == 0xC0 || ch1 == 0xC1) {
			goto inval_lead;
		}
		nc = 1;
	} else if ((ch1 & 0xF0) == 0xE0) {
		nc = 2;
	} else if ((ch1 & 0xFC) == 0xF0 || ch1 == 0xF4) {
		nc = 3;
	} else {
		// expecting bytes in the following ranges: 00..7F C2..F4
		goto inval_lead;
	}

	// ensure string is long enough
	if (ptr + nc > end) {
		// expecting another continuation byte
		goto inval_incomplete;
	}

	/* First Continuation byte
	 * -----------
	 * X  + 80..BF:
	 * 80: 1000 0000
	 * BF: 1011 1111
	 * (ch & 0xC0 == 0x80)
	 *
	 * E0 + A0..BF:
	 * A0: 1010 0000
	 * BF: 1011 1111
	 * (ch & 0xE0 == 0xA0)
	 *
	 * ED + 80..9F:
	 * 80: 1000 0000
	 * 9F: 1001 1111
	 * (ch & 0xE0 == 0x80)
	 *
	 * F0 + 90..BF:
	 * 90: 1001 0000
	 * BF: 1011 1111
	 * (ch & 0xF0 == 0x90 || ch & 0xE0 == A0)
	 *
	 */

	// validate the first continuation byte
	ch = *ptr++;
	switch (ch1) {
	case 0xE0:
		if ((ch & 0xE0) != 0xA0) {
			// expecting a byte between A0 and BF
			goto inval_cont;
		}
		break;
	case 0xED:
		if ((ch & 0xE0) != 0x80) {
			// expecting a byte between A0 and 9F
			goto inval_cont;
		}
		break;
	case 0xF0:
		if ((ch & 0xE0) != 0xA0 && (ch & 0xF0) != 0x90) {
			// expecting a byte between 90 and BF
			goto inval_cont;
		}
		break;
	case 0xF4:
		if ((ch & 0xF0) != 0x80) {
			// expecting a byte between 80 and 8F
			goto inval_cont;
		}
	default:
		if ((ch & 0xC0) != 0x80) {
			// expecting a byte between 80 and BF
			goto inval_cont;
		}
		break;
	}
	nc--;

	// validate the trailing continuation bytes
	while (nc-- > 0) {
		ch = *ptr++;
		if ((ch & 0xC0) != 0x80) {
			// expecting a byte between 80 and BF
			goto inval_cont;
		}
	}

success:
	err = 0;
	goto out;

inval_incomplete:
	utf8lite_message_set(msg, "not enough continuation bytes"
			     " after leading byte (0x%02X)",
			     (unsigned)ch1);
	goto error;

inval_lead:
	utf8lite_message_set(msg, "invalid leading byte (0x%02X)",
			     (unsigned)ch1);
	goto error;

inval_cont:
	utf8lite_message_set(msg, "leading byte 0x%02X followed by"
			     " invalid continuation byte (0x%02X)",
			     (unsigned)ch1, (unsigned)ch);
	goto error;

error:
	ptr--;
	err = UTF8LITE_ERROR_INVAL;
out:
	*bufptr = ptr;
	return err;
}


void utf8lite_decode_utf8(const uint8_t **bufptr, int32_t *codeptr)
{
	const uint8_t *ptr = *bufptr;
	int32_t code;
	uint_fast8_t ch;
	unsigned nc;

	ch = *ptr++;
	if (!(ch & 0x80)) {
		code = ch;
		nc = 0;
	} else if (!(ch & 0x20)) {
		code = ch & 0x1F;
		nc = 1;
	} else if (!(ch & 0x10)) {
		code = ch & 0x0F;
		nc = 2;
	} else {
		code = ch & 0x07;
		nc = 3;
	}

	while (nc-- > 0) {
		ch = *ptr++;
		code = (code << 6) + (ch & 0x3F);
	}

	*bufptr = ptr;
	*codeptr = code;
}


// http://www.fileformat.info/info/unicode/utf8.htm
void utf8lite_encode_utf8(int32_t code, uint8_t **bufptr)
{
	uint8_t *ptr = *bufptr;
	int32_t x = code;

	if (x <= 0x7F) {
		*ptr++ = (uint8_t)x;
	} else if (x <= 0x07FF) {
		*ptr++ = (uint8_t)(0xC0 | (x >> 6));
		*ptr++ = (uint8_t)(0x80 | (x & 0x3F));
	} else if (x <= 0xFFFF) {
		*ptr++ = (uint8_t)(0xE0 | (x >> 12));
		*ptr++ = (uint8_t)(0x80 | ((x >> 6) & 0x3F));
		*ptr++ = (uint8_t)(0x80 | (x & 0x3F));
	} else {
		*ptr++ = (uint8_t)(0xF0 | (x >> 18));
		*ptr++ = (uint8_t)(0x80 | ((x >> 12) & 0x3F));
		*ptr++ = (uint8_t)(0x80 | ((x >> 6) & 0x3F));
		*ptr++ = (uint8_t)(0x80 | (x & 0x3F));
	}

	*bufptr = ptr;
}


void utf8lite_rencode_utf8(int32_t code, uint8_t **bufptr)
{
	uint8_t *ptr = *bufptr;
	int32_t x = code;

	if (x <= 0x7F) {
		*--ptr = (uint8_t)x;
	} else if (x <= 0x07FF) {
		*--ptr = (uint8_t)(0x80 | (x & 0x3F));
		*--ptr = (uint8_t)(0xC0 | (x >> 6));
	} else if (x <= 0xFFFF) {
		*--ptr = (uint8_t)(0x80 | (x & 0x3F));
		*--ptr = (uint8_t)(0x80 | ((x >> 6) & 0x3F));
		*--ptr = (uint8_t)(0xE0 | (x >> 12));
	} else {
		*--ptr = (uint8_t)(0x80 | (x & 0x3F));
		*--ptr = (uint8_t)(0x80 | ((x >> 6) & 0x3F));
		*--ptr = (uint8_t)(0x80 | ((x >> 12) & 0x3F));
		*--ptr = (uint8_t)(0xF0 | (x >> 18));
	}

	*bufptr = ptr;
}
