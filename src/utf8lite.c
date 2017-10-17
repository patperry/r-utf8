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
#include <errno.h>
#include <stdint.h>
#include <stdlib.h>
#include "private/casefold.h"
#include "private/charwidth.h"
#include "private/compose.h"
#include "private/combining.h"
#include "private/decompose.h"
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


int utf8lite_scan_utf8(const uint8_t **bufptr, const uint8_t *end)
{
	const uint8_t *ptr = *bufptr;
	uint_fast8_t ch, ch1;
	unsigned nc;
	int err;

	if (ptr >= end) {
		return EINVAL;
	}

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
		goto backtrack;
	} else if ((ch1 & 0xE0) == 0xC0) {
		if (ch1 == 0xC0 || ch1 == 0xC1) {
			goto backtrack;
		}
		nc = 1;
	} else if ((ch1 & 0xF0) == 0xE0) {
		nc = 2;
	} else if ((ch1 & 0xFC) == 0xF0 || ch1 == 0xF4) {
		nc = 3;
	} else {
		// expecting bytes in the following ranges: 00..7F C2..F4
		goto backtrack;
	}

	// ensure string is long enough
	if (ptr + nc > end) {
		// expecting another continuation byte
		goto backtrack;
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
			goto backtrack;
		}
		break;
	case 0xED:
		if ((ch & 0xE0) != 0x80) {
			// expecting a byte between A0 and 9F
			goto backtrack;
		}
		break;
	case 0xF0:
		if ((ch & 0xE0) != 0xA0 && (ch & 0xF0) != 0x90) {
			// expecting a byte between 90 and BF
			goto backtrack;
		}
		break;
	case 0xF4:
		if ((ch & 0xF0) != 0x80) {
			// expecting a byte between 80 and 8F
			goto backtrack;
		}
	default:
		if ((ch & 0xC0) != 0x80) {
			// expecting a byte between 80 and BF
			goto backtrack;
		}
		break;
	}
	nc--;

	// validate the trailing continuation bytes
	while (nc-- > 0) {
		ch = *ptr++;
		if ((ch & 0xC0) != 0x80) {
			// expecting a byte between 80 and BF
			goto backtrack;
		}
	}

success:
	err = 0;
	goto out;
backtrack:
	ptr--;
	err = EINVAL;
out:
	*bufptr = ptr;
	return err;
}


void utf8lite_decode_utf8(const uint8_t **bufptr, uint32_t *codeptr)
{
	const uint8_t *ptr = *bufptr;
	uint32_t code;
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
void utf8lite_encode_utf8(uint32_t code, uint8_t **bufptr)
{
	uint8_t *ptr = *bufptr;
	uint32_t x = code;

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


void utf8lite_rencode_utf8(uint32_t code, uint8_t **bufptr)
{
	uint8_t *ptr = *bufptr;
	uint32_t x = code;

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


/* From Unicode-8.0 Section 3.12 Conjoining Jamo Behavior */
#define HANGUL_SBASE 0xAC00
#define HANGUL_LBASE 0x1100
#define HANGUL_VBASE 0x1161
#define HANGUL_TBASE 0x11A7
#define HANGUL_LCOUNT 19
#define HANGUL_VCOUNT 21
#define HANGUL_TCOUNT 28
#define HANGUL_NCOUNT (HANGUL_VCOUNT * HANGUL_TCOUNT)
#define HANTUL_SCOUNT (HANGUL_LCOUNT * HANGUL_NCOUNT)


static void hangul_decompose(uint32_t code, uint32_t **bufp)
{
	uint32_t *dst = *bufp;
	uint32_t sindex = code - HANGUL_SBASE;
	uint32_t lindex = sindex / HANGUL_NCOUNT;
	uint32_t vindex = (sindex % HANGUL_NCOUNT) / HANGUL_TCOUNT;
	uint32_t tindex = sindex % HANGUL_TCOUNT;
	uint32_t lpart = HANGUL_LBASE + lindex;
	uint32_t vpart = HANGUL_VBASE + vindex;
	uint32_t tpart = HANGUL_TBASE + tindex;

	*dst++ = lpart;
	*dst++ = vpart;
	if (tindex > 0) {
		*dst++ = tpart;
	}

	*bufp = dst;
}


static int is_hangul_vpart(uint32_t code)
{
	return (HANGUL_VBASE <= code && code < HANGUL_VBASE + HANGUL_VCOUNT);
}


static int is_hangul_tpart(uint32_t code)
{
	// strict less-than on lower bound
	return (HANGUL_TBASE < code && code < HANGUL_TBASE + HANGUL_TCOUNT);
}


static uint32_t hangul_compose_lv(uint32_t lpart, uint32_t vpart)
{
	uint32_t lindex = lpart - HANGUL_LBASE;
	uint32_t vindex = vpart - HANGUL_VBASE;
	uint32_t lvindex = lindex * HANGUL_NCOUNT + vindex * HANGUL_TCOUNT;
	uint32_t s = HANGUL_SBASE + lvindex;
	return s;
}


static uint32_t hangul_compose_lvt(uint32_t lvpart, uint32_t tpart)
{
	uint32_t tindex = tpart - HANGUL_TBASE;
	uint32_t s = lvpart + tindex;
	return s;
}


static void casefold(int type, uint32_t code, uint32_t **bufp)
{
	const uint32_t block_size = CASEFOLD_BLOCK_SIZE;
	unsigned i = casefold_stage1[code / block_size];
	struct casefold c = casefold_stage2[i][code % block_size];
	unsigned length = c.length;
	const uint32_t *src;
	uint32_t *dst;

	if (length == 0) {
		dst = *bufp;
		*dst++ = code;
		*bufp = dst;
	} else if (length == 1) {
		utf8lite_map(type, c.data, bufp);
	} else {
		src = &casefold_mapping[c.data];
		while (length-- > 0) {
			utf8lite_map(type, *src, bufp);
			src++;
		}
	}
}


void utf8lite_map(int type, uint32_t code, uint32_t **bufptr)
{
	const uint32_t block_size = DECOMPOSITION_BLOCK_SIZE;
	unsigned i = decomposition_stage1[code / block_size];
	struct decomposition d = decomposition_stage2[i][code % block_size];
	unsigned length = d.length;
	const uint32_t *src;
	uint32_t *dst;

	if (length == 0 || (d.type > 0 && !(type & (1 << (d.type - 1))))) {
		if (type & UTF8LITE_CASEFOLD_ALL) {
			casefold(type, code, bufptr);
		} else {
			dst = *bufptr;
			*dst++ = code;
			*bufptr = dst;
		}
	} else if (length == 1) {
		utf8lite_map(type, d.data, bufptr);
	} else if (d.type >= 0) {
		src = &decomposition_mapping[d.data];
		while (length-- > 0) {
			utf8lite_map(type, *src, bufptr);
			src++;
		}
	} else {
		hangul_decompose(code, bufptr);
	}
}


void utf8lite_order(uint32_t *ptr, size_t len)
{
	uint32_t *end = ptr + len;
	uint32_t *c_begin, *c_end, *c_tail, *c_ptr;
	uint32_t code, code_prev;
	uint32_t cl, cl_prev;

	while (ptr != end) {
		c_begin = ptr;
		code = *ptr++;
		cl = combining_class(code);

		// skip to the next combining mark
		if (cl == 0) {
			continue;
		}

		// mark the start of the combining mark sequence (c_begin)
		// encode the combining class in the high 8 bits
		*c_begin = code | (cl << 24);

		// the combining mark sequence ends at the first starter
		// (c_end)
		c_end = ptr;
		while (c_end != end) {
			// until we hit a non-starter, encode the combining
			// class in the high 8 bits of the code
			code = *ptr++;
			cl = combining_class(code);
			if (cl == 0) {
				break;
			}

			*c_end = code | (cl << 24);
			c_end++;
		}

		// sort the combining marks, using insertion sort (stable)
		for (c_tail = c_begin + 1; c_tail != c_end; c_tail++) {
			c_ptr = c_tail;
			code = *c_ptr;
			cl = code & (0xFFU << 24);

			while (c_ptr != c_begin) {
				code_prev = c_ptr[-1];
				cl_prev = code_prev & (0xFFU << 24);

				if (cl_prev <= cl) {
					break;
				}

				// swap with previous item
				c_ptr[0] = code_prev;

				// move down
				c_ptr--;
			}

			// complete the final swap
			*c_ptr = code;
		}

		// remove the combining mark annotations
		while (c_begin != c_end) {
			code = *c_begin;
			*c_begin = code & (~(0xFFU << 24));
			c_begin++;
		}
	}
}


static int has_compose(uint32_t code, int *offsetptr, int *lengthptr)
{
	const uint32_t block_size = COMPOSITION_BLOCK_SIZE;
	unsigned i = composition_stage1[code / block_size];
	struct composition c = composition_stage2[i][code % block_size];
	int offset = (int)c.offset;
	int length = (int)c.length;

	*offsetptr = offset;
	*lengthptr = length;

	return (length > 0 ? 1 : 0);
}


static int code_cmp(const void *x1, const void *x2)
{
	uint32_t y1 = *(const uint32_t *)x1;
	uint32_t y2 = *(const uint32_t *)x2;

	if (y1 < y2) {
		return -1;
	} else if (y1 > y2) {
		return +1;
	} else {
		return 0;
	}
}


static int combiner_find(int offset, int length, uint32_t code)
{
	const uint32_t *base = composition_combiner + offset;
	const uint32_t *ptr;

	// handle empty and singleton case
	if (length == 0) {
		return -1;
	} else if (length == 1) {
		return (*base == code) ? 0 : -1;
	}

	// handle general case
	ptr = bsearch(&code, base, (size_t)length, sizeof(*base), code_cmp);

	if (ptr == NULL) {
		return -1;
	} else {
		return (int)(ptr - base);
	}
}


static int has_combiner(uint32_t left, int offset, int length, uint32_t code,
		        uint32_t *primaryptr)
{
	int i;

	if (offset < COMPOSITION_HANGUL_LPART) {
		i = combiner_find(offset, length, code);
		if (i >= 0) {
			*primaryptr = composition_primary[offset + i];
			return 1;
		}
	} else if (offset == COMPOSITION_HANGUL_LPART) {
		if (is_hangul_vpart(code)) {
			*primaryptr = hangul_compose_lv(left, code);
			return 1;
		}
	} else if (offset == COMPOSITION_HANGUL_LVPART) {
		if (is_hangul_tpart(code)) {
			*primaryptr = hangul_compose_lvt(left, code);
			return 1;
		}
	}

	return 0;
}


void utf8lite_compose(uint32_t *ptr, size_t *lenptr)
{
	size_t len = *lenptr;
	uint32_t *begin = ptr;
	uint32_t *end = begin + len;
	uint32_t *leftptr, *dst;
	uint32_t left = 0, code, prim;
	uint8_t code_ccc, prev_ccc = 0;
	int moff = 0, mlen = 0;
	int blocked, has_prev, did_del;

	did_del = 0;

	// find the first combining starter (the left code point, L)
	leftptr = begin;
	while (leftptr != end) {
		left = *leftptr;
		if (has_compose(left, &moff, &mlen)) {
			break;
		}
		leftptr++;
	}

	if (leftptr == end) {
		goto out;
	}

	ptr = leftptr + 1;
	has_prev = 0;
	while (ptr != end) {
		code = *ptr;
		code_ccc = combining_class(code);

		// determine whether the code is blocked
		if (has_prev && prev_ccc >= code_ccc) {
			blocked = 1;
		} else {
			blocked = 0;
		}

		if (!blocked && has_combiner(left, moff, mlen, code, &prim)) {
			// replace L by P
			*leftptr = prim;
			left = prim;
			has_compose(left, &moff, &mlen);

			// delete C
			*ptr = UINT32_MAX;
			did_del = 1;
		} else if (code_ccc == 0) {
			// new leftmost combining starter, L
			leftptr = ptr;
			left = code;
			has_compose(left, &moff, &mlen);
			has_prev = 0;
		} else {
			prev_ccc = code_ccc;
			has_prev = 1;
		}
		ptr++;
	}

	// remove the deleted entries
	if (did_del) {
		ptr = begin;
		dst = begin;
		while (ptr != end) {
			code = *ptr++;
			if (code != UINT32_MAX) {
				*dst++ = code;
			}
		}
		len = (size_t)(dst - begin);
	}

out:
	*lenptr = len;
}


int utf8lite_charwidth(uint32_t code)
{
	int prop = charwidth(code);
	switch(prop) {
	case CHARWIDTH_OTHER:
		return UTF8LITE_CHARWIDTH_OTHER;
	case CHARWIDTH_EMOJI:
		return UTF8LITE_CHARWIDTH_EMOJI;
	case CHARWIDTH_AMBIGUOUS:
		return UTF8LITE_CHARWIDTH_AMBIGUOUS;
	case CHARWIDTH_IGNORABLE:
		return UTF8LITE_CHARWIDTH_IGNORABLE;
	case CHARWIDTH_NONE:
		return UTF8LITE_CHARWIDTH_NONE;
	case CHARWIDTH_NARROW:
		return UTF8LITE_CHARWIDTH_NARROW;
	case CHARWIDTH_WIDE:
		return UTF8LITE_CHARWIDTH_WIDE;
	default:
		assert(0 && "internal error: unrecognized charwidth property");
		return UTF8LITE_CHARWIDTH_OTHER;
	}
}

// TODO: use character class lookup table
int utf8lite_isspace(uint32_t code)
{
	if (code <= 0x7F) {
		return (code == 0x20 || (0x09 <= code && code < 0x0E));
	} else if (code <= 0x1FFF) {
		switch (code) {
		case 0x0085:
		case 0x00A0:
		case 0x1680:
			return 1;
		default:
			return 0;
		}
	} else if (code <= 0x200A) {
		return 1;
	} else if (code <= 0x3000) {
		switch (code) {
		case 0x2028:
		case 0x2029:
		case 0x202F:
		case 0x205F:
		case 0x3000:
			return 1;
		default:
			return 0;
		}
	} else {
		return 0;
	}
}

// TODO use lookup table
int utf8lite_isignorable(uint32_t code)
{
	// Default_Ignorable_Code_Point = Yes
	if (code <= 0x7F) {
		return 0;
	}

	switch (code) {
	case 0x00AD:
	case 0x034F:
	case 0x061C:
	case 0x115F:
	case 0x1160:
	case 0x17B4:
	case 0x17B5:
	case 0x180B:
	case 0x180C:
	case 0x180D:
	case 0x180E:
	case 0x200B:
	case 0x200C:
	case 0x200D:
	case 0x200E:
	case 0x200F:
	case 0x202A:
	case 0x202B:
	case 0x202C:
	case 0x202D:
	case 0x202E:
	case 0x2060:
	case 0x2061:
	case 0x2062:
	case 0x2063:
	case 0x2064:
	case 0x2065:
	case 0x2066:
	case 0x2067:
	case 0x2068:
	case 0x2069:
	case 0x206A:
	case 0x206B:
	case 0x206C:
	case 0x206D:
	case 0x206E:
	case 0x206F:
	case 0x3164:
	case 0xFE00:
	case 0xFE01:
	case 0xFE02:
	case 0xFE03:
	case 0xFE04:
	case 0xFE05:
	case 0xFE06:
	case 0xFE07:
	case 0xFE08:
	case 0xFE09:
	case 0xFE0A:
	case 0xFE0B:
	case 0xFE0C:
	case 0xFE0D:
	case 0xFE0E:
	case 0xFE0F:
	case 0xFEFF:
	case 0xFFA0:
	case 0xFFF0:
	case 0xFFF1:
	case 0xFFF2:
	case 0xFFF3:
	case 0xFFF4:
	case 0xFFF5:
	case 0xFFF6:
	case 0xFFF7:
	case 0xFFF8:
	case 0x1BCA0:
	case 0x1BCA1:
	case 0x1BCA2:
	case 0x1BCA3:
	case 0x1D173:
	case 0x1D174:
	case 0x1D175:
	case 0x1D176:
	case 0x1D177:
	case 0x1D178:
	case 0x1D179:
	case 0x1D17A:
		return 1;
	default:
		return (0xDFFFF < code && code <= 0xE0FFF);
	}
}
