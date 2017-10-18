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

#include <stdlib.h>
#include "private/casefold.h"
#include "private/compose.h"
#include "private/combining.h"
#include "private/decompose.h"
#include "utf8lite.h"

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
