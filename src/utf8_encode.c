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
#include <stddef.h>
#include "rutf8.h"

/* Some displays will wrongly format emoji with one space instead of two.
 * Fortunately, these displays also give zero-width-spaces one space
 * instead of zero.
 *
 * Append a zero-width space will fix the emoji display problems on these
 * terminals, while leaving other terminals unaffected.
 */

#define ZWSP "\xE2\x80\x8B" // U+200B
#define ZWSP_NBYTE 3


static int needs_encode_chars(const uint8_t *str, size_t size0, int display,
			      int utf8, int *sizeptr)
{
	const uint8_t *end = str + size0;
	const uint8_t *ptr = str;
	const uint8_t *start;
	int32_t code;
	int cw, err, needs, nbyte, size;

	size = 0;
	needs = 0;
	while (ptr != end) {
		nbyte = 1 + UTF8LITE_UTF8_TAIL_LEN(*ptr);
		start = ptr;
		if ((err = utf8lite_scan_utf8(&start, end, NULL))) {
			// encode invalid byte as \xHH (4 bytes)
			needs = 1;
			nbyte = 4;
			ptr++;
		} else if (nbyte == 1) { // code < 0x80
			code = *ptr++;
			switch (code) {
			case '\a':
			case '\b':
			case '\f':
			case '\n':
			case '\r':
			case '\t':
			case '\v':
				needs = 1;
				nbyte = 2;
				break;
			default:
				if (!isprint((int)code)) {
					needs = 1;
					nbyte = 6; // \uXXXX
				}
				break;
			}
		} else {
			utf8lite_decode_utf8(&ptr, &code);
			if (utf8) {
				cw = charwidth(code);
				switch (cw) {
				case UTF8LITE_CHARWIDTH_NONE:
					// \uXXXX or \UXXXXYYYY
					needs = 1;
					nbyte = ((code <= 0xFFFF) ? 6 : 10);
					break;

				case UTF8LITE_CHARWIDTH_IGNORABLE:
					if (display) {
						// remove ignorables
						needs = 1;
						nbyte = 0;
					}
					break;

				case UTF8LITE_CHARWIDTH_EMOJI:
					if (display) {
						// add zwsp after emoji
						needs = 1;
						nbyte += ZWSP_NBYTE;
					}
					break;

				default:
					break;
				}
			} else {
				needs = 1;
				// \uXXXX or \UXXXXYYYY
				nbyte = (code <= 0xFFFF) ? 6 : 10;
			}
		}

		if (size > INT_MAX - nbyte) {
			error("encoded character string size",
			      " exceeds maximum (2^31-1 bytes)");
		}
		size += nbyte;
	}
	if (sizeptr) {
		*sizeptr = size;
	}

	return needs;
}


static void encode_chars(uint8_t *dst, const uint8_t *str, size_t size,
			 int display, int utf8)
{
	const uint8_t *end = str + size;
	const uint8_t *ptr = str;
	const uint8_t *start;
	int32_t code;
	int cw, err, nbyte;

	while (ptr != end) {
		start = ptr;
		if ((err = utf8lite_scan_utf8(&start, end, NULL))) {
			sprintf((char *)dst, "\\x%02x", (unsigned)*ptr);
			dst += 4;
			ptr++;
			continue;
		}

		start = ptr;
		nbyte = 1 + UTF8LITE_UTF8_TAIL_LEN(*ptr);

		if (nbyte == 1) { // code < 0x80
			code = *ptr++;
			switch (code) {
			case '\a':
				*dst++ = '\\';
				*dst++ = 'a';
				break;
			case '\b':
				*dst++ = '\\';
				*dst++ = 'b';
				break;
			case '\f':
				*dst++ = '\\';
				*dst++ = 'f';
				break;
			case '\n':
				*dst++ = '\\';
				*dst++ = 'n';
				break;
			case '\r':
				*dst++ = '\\';
				*dst++ = 'r';
				break;
			case '\t':
				*dst++ = '\\';
				*dst++ = 't';
				break;
			case '\v':
				*dst++ = '\\';
				*dst++ = 'v';
				break;
			default:
				if (!isprint((int)code)) {
					sprintf((char *)dst, "\\u%04x",
						(unsigned)code);
					dst += 6;
				} else {
					*dst++ = (uint8_t)code;
				}
				break;
			}
			continue;
		}

		utf8lite_decode_utf8(&ptr, &code);
		cw = charwidth(code);

		if (cw == UTF8LITE_CHARWIDTH_NONE || !utf8) {
			if (code <= 0xFFFF) {
				sprintf((char *)dst, "\\u%04x",
					(unsigned)code);
				dst += 6;
			} else {
				sprintf((char *)dst, "\\U%08x",
					(unsigned)code);
				dst += 10;
			}
			continue;
		}

		if (cw == UTF8LITE_CHARWIDTH_IGNORABLE && display) {
			continue;
		}

		while (nbyte-- > 0) {
			*dst++ = *start++;
		}

		if (cw == UTF8LITE_CHARWIDTH_EMOJI && display) {
			memcpy(dst, ZWSP, ZWSP_NBYTE);
			dst += ZWSP_NBYTE;
		}
	}
}


static int needs_encode_bytes(const uint8_t *str, size_t size0, int *sizeptr)
{
	const uint8_t *end = str + size0;
	const uint8_t *ptr = str;
	int code;
	int needs, nbyte, size;

	size = 0;
	needs = 0;
	while (ptr != end) {
		code = *ptr++;
		switch (code) {
		case '\a':
		case '\b':
		case '\f':
		case '\n':
		case '\r':
		case '\t':
		case '\v':
			needs = 1;
			nbyte = 2;
			break;
		default:
			if (code < 0x80 && isprint(code)) {
				nbyte = 1;
			} else {
				needs = 1;
				nbyte = 4; // \xXX
			}
			break;
		}

		if (size > INT_MAX - nbyte) {
			error("encoded character string size",
			      " exceeds maximum (2^31-1 bytes)");
		}
		size += nbyte;
	}
	if (sizeptr) {
		*sizeptr = size;
	}

	return needs;
}


static void encode_bytes(uint8_t *dst, const uint8_t *str, size_t size)
{
	const uint8_t *end = str + size;
	const uint8_t *ptr = str;
	uint8_t code;

	while (ptr != end) {
		code = *ptr++;
		switch (code) {
		case '\a':
			*dst++ = '\\';
			*dst++ = 'a';
			break;
		case '\b':
			*dst++ = '\\';
			*dst++ = 'b';
			break;
		case '\f':
			*dst++ = '\\';
			*dst++ = 'f';
			break;
		case '\n':
			*dst++ = '\\';
			*dst++ = 'n';
			break;
		case '\r':
			*dst++ = '\\';
			*dst++ = 'r';
			break;
		case '\t':
			*dst++ = '\\';
			*dst++ = 't';
			break;
		case '\v':
			*dst++ = '\\';
			*dst++ = 'v';
			break;
		default:
			if (code < 0x80 && isprint((int)code)) {
				*dst++ = code;
			} else {
				sprintf((char *)dst, "\\x%02x",
					(unsigned)code);
				dst += 4;
			}
			break;
		}
	}
}



static SEXP charsxp_encode(SEXP sx, int display, int utf8, char **bufptr,
			   int *nbufptr)
{
	const uint8_t *str, *str2;
	char *buf = *bufptr;
	int nbuf = *nbufptr;
	int conv, size, size2;
	cetype_t ce;

	if (sx == NA_STRING) {
		return NA_STRING;
	}

	str = (const uint8_t *)CHAR(sx);
	size = (size_t)XLENGTH(sx);
	conv = 0;

	ce = getCharCE(sx);
	if (!encodes_utf8(ce) && ce != CE_BYTES) {
		str2 = (const uint8_t *)translate_utf8(sx);
		ce = CE_UTF8;
		if (str2 != str) {
			str = str2;
			size = strlen((const char *)str);
			conv = 1;
		}
	}

	if (ce == CE_BYTES) {
		if (!needs_encode_bytes(str, size, &size2)) {
			return sx;
		}
	} else if (!needs_encode_chars(str, size, display, utf8, &size2)) {
		if (conv) {
			sx = mkCharLenCE((const char *)str, size, CE_UTF8);
		}
		return sx;
	}

	if (size2 > nbuf) {
		array_size_add(&nbuf, 1, 0, size2);
		buf = R_alloc(nbuf, 1);
		*bufptr = buf;
		*nbufptr = nbuf;
	}

	if (ce == CE_BYTES) {
		encode_bytes((uint8_t *)buf, str, size);
	} else {
		encode_chars((uint8_t *)buf, str, size, display, utf8);
	}

	return mkCharLenCE(buf, size2, CE_UTF8);
}


SEXP rutf8_utf8_encode(SEXP sx, SEXP swidth, SEXP squote,
		       SEXP sjustify, SEXP sdisplay, SEXP sutf8)
{
	SEXP ans, elt, elt2;
	enum rutf8_justify_type justify;
	char *buf;
	R_xlen_t i, n;
	int duped = 0, width, quote, display, utf8, nbuf;

	if (sx == R_NilValue) {
		return R_NilValue;
	}

	if (!isString(sx)) {
		error("argument is not a character object");
	}

	if (swidth == R_NilValue || INTEGER(swidth)[0] == NA_INTEGER) {
		width = -1;
	} else {
		width = INTEGER(swidth)[0];
	}

	quote = LOGICAL(squote)[0] == TRUE;
	justify = rutf8_as_justify(sjustify);
	display = LOGICAL(sdisplay)[0] == TRUE;
	utf8 = LOGICAL(sutf8)[0] == TRUE;

	n = XLENGTH(sx);
	ans = sx;
	buf = NULL;
	nbuf = 0;

	for (i = 0; i < n; i++) {
		CHECK_INTERRUPT(i);

		elt = STRING_ELT(ans, i);
		elt2 = charsxp_encode(elt, display, utf8, &buf, &nbuf);
		if (elt != elt2) {
			if (!duped) {
				PROTECT(elt2);
				PROTECT(ans = duplicate(ans));
				duped = 1;
			}
			SET_STRING_ELT(ans, i, elt2);
		}
	}

	if (duped) {
		UNPROTECT(2);
	}
	return ans;
}
