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
#include <ctype.h>
#include <string.h>
#include "rutf8.h"


int rutf8_as_justify(SEXP justify)
{
	const char *str;

	str = CHAR(STRING_ELT(justify, 0));
	if (strcmp(str, "left") == 0) {
		return RUTF8_JUSTIFY_LEFT;
	} else if (strcmp(str, "right") == 0) {
		return RUTF8_JUSTIFY_RIGHT;
	} else if (strcmp(str, "centre") == 0) {
		return RUTF8_JUSTIFY_CENTRE;
	} else {
		return RUTF8_JUSTIFY_NONE;
	}
}


int centre_pad_begin(struct utf8lite_render *r, int width_max, int fullwidth)
{
	int fill, bfill = 0;

	if ((fill = width_max - fullwidth) > 0) {
		bfill = fill / 2;
		pad_spaces(r, bfill);
	}
	return bfill;
}


int charwidth(uint32_t code)
{
#if (defined(_WIN32) || defined(_WIN64))
	if (code > 0xFFFF) {
		return UTF8LITE_CHARWIDTH_NONE;
	}
#endif
	return utf8lite_charwidth(code);
}


int charsxp_width(SEXP charsxp, int quote, int utf8)
{
	const uint8_t *ptr = (const uint8_t *)CHAR(charsxp);
	const uint8_t *start;
	const uint8_t *end = ptr + XLENGTH(charsxp);
	int32_t code;
	int width, cw, err;

	width = quote ? 2 : 0;

	while (ptr != end) {
		start = ptr;
		if ((err = utf8lite_scan_utf8(&start, end, NULL))) {
			return NA_INTEGER;
		}

		utf8lite_decode_utf8(&ptr, &code);
		if (code < 0x80) {
			switch (code) {
			case '\a':
			case '\b':
			case '\f':
			case '\n':
			case '\r':
			case '\t':
			case '\v':
				return NA_INTEGER;
			case '"':
				width += (quote ? 2 : 1);
				continue;
			default:
				if (!isprint((int)code)) {
					return NA_INTEGER;
				}
				width += 1;
				continue;
			}
		} else if (!utf8) {
			return NA_INTEGER;
		}

		cw = charwidth(code);

		switch (cw) {
		case UTF8LITE_CHARWIDTH_NONE:
		case UTF8LITE_CHARWIDTH_IGNORABLE:
			break;

		case UTF8LITE_CHARWIDTH_NARROW:
		case UTF8LITE_CHARWIDTH_AMBIGUOUS:
			width += 1;
			break;

		case UTF8LITE_CHARWIDTH_WIDE:
		case UTF8LITE_CHARWIDTH_EMOJI:
			width += 2;
			break;

		default: // UTF8LITE_CHARWIDTH_NONE
			return NA_INTEGER;
		}
	}

	return width;
}


int encodes_utf8(cetype_t ce)
{
	switch (ce) {
	case CE_ANY:
	case CE_UTF8:
#if (!defined(_WIN32) && !defined(_WIN64))
	case CE_NATIVE: // assume that 'native' is UTF-8 on non-Windows
#endif
		return 1;
	default:
		return 0;
	}
}


void pad_spaces(struct utf8lite_render *r, int nspace)
{
	int err = 0;
	while (nspace-- > 0) {
		TRY(utf8lite_render_string(r, " "));
	}
exit:
	CHECK_ERROR(err);
}


#if (defined(_WIN32) || defined(_WIN64))
#include <windows.h>
extern unsigned int localeCP;

const char *translate_utf8(SEXP x)
{
	LPWSTR wstr;
	const char *raw;
	char *str;
	cetype_t ce;
	int len, wlen, n;
	UINT cp;

	ce = getCharCE(x);
	raw = CHAR(x);
	n = LENGTH(x);


	if (ce == CE_ANY || ce == CE_UTF8 || n == 0) {
		return raw;
	}

	assert(ce == CE_NATIVE || ce == CE_LATIN1);

	if (ce == CE_LATIN1) {
		// R seems to mark native strings as "latin1" when the code page
		// is set to 1252, but this doesn't seem to be correct. Work
		// around this behavior by decoding "latin1" as Windows-1252.
		cp = 1252;
	} else {
		cp = localeCP;
		if (cp == 0) {
			// Failed determining code page from locale. Use native
			// code page, which R interprets to be the ANSI Code Page
			// **not GetConsoleCP(), even if CharacterMode == RTerm**.
			// See src/extra/win_iconv.c; name_to_codepage().
			cp = GetACP();
		}
	}

	// translate from current code page to UTF-16
	wlen = MultiByteToWideChar(cp, 0, raw, n, NULL, 0);
	wstr = (LPWSTR)R_alloc(wlen, sizeof(*wstr));
	MultiByteToWideChar(cp, 0, raw, n, wstr, wlen);

	// convert from UTF-16 to UTF-8
	len = WideCharToMultiByte(CP_UTF8, 0, wstr, wlen, NULL, 0, NULL, NULL);
	str = R_alloc(len + 1, 1); // add space for NUL
	WideCharToMultiByte(CP_UTF8, 0, wstr, wlen, str, len, NULL, NULL);
	str[len] = '\0';

	return str;
}

#else

const char *translate_utf8(SEXP x)
{
	return translateCharUTF8(x);
}

#endif
