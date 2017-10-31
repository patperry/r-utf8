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
#include <inttypes.h>
#include <stddef.h>
#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include "utf8lite/src/utf8lite.h"
#include "rutf8.h"


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

		default: // UTF8LITE_CHARWIDTH_OTHER
			return NA_INTEGER;
		}
	}

	return width;
}


static const char *encoding_name(cetype_t ce)
{
	switch (ce) {
	case CE_LATIN1:
		return "latin1";
	case CE_UTF8:
		return "UTF-8";
	case CE_SYMBOL:
		return "symbol";
	case CE_BYTES:
		return "bytes";
	case CE_ANY:
	case CE_NATIVE:
	default:
		return "unknown";
	}
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


static int is_valid(const uint8_t *str, size_t size, size_t *errptr)
{
	const uint8_t *end = str + size;
	const uint8_t *start;
	const uint8_t *ptr = str;
	size_t err = (size_t)-1;
	int valid;

	valid = 1;
	while (ptr != end) {
		start = ptr;
		if (utf8lite_scan_utf8(&ptr, end, NULL)) {
			err = (size_t)(start - str);
			valid = 0;
			goto out;
		}
	}

out:
	if (!valid && errptr) {
		*errptr = err;
	}

	return valid;
}


int charwidth(uint32_t code)
{
#if (defined(_WIN32) || defined(_WIN64))
	if (code > 0xFFFF) {
		return UTF8LITE_CHARWIDTH_OTHER;
	}
#endif
	return utf8lite_charwidth(code);
}


SEXP rutf8_utf8_coerce(SEXP sx)
{
	SEXP ans, sstr;
	const uint8_t *str;
	cetype_t ce;
	size_t size, err;
	R_xlen_t i, n;
	int raw, duped;;

	if (sx == R_NilValue) {
		return R_NilValue;
	}
	if (!isString(sx)) {
		error("argument is not a character object");
	}

	ans = sx;
	duped = 0;

	n = XLENGTH(sx);
	for (i = 0; i < n; i++) {
		CHECK_INTERRUPT(i);

		sstr = STRING_ELT(sx, i);
		if (sstr == NA_STRING) {
			continue;
		}

		ce = getCharCE(sstr);
		raw = encodes_utf8(ce) || ce == CE_BYTES;

		if (raw) {
			str = (const uint8_t *)CHAR(sstr);
			size = (size_t)XLENGTH(sstr);
		} else {
			str = (const uint8_t *)translate_utf8(sstr);
			size = strlen((const char *)str);
		}

		if (!is_valid(str, size, &err)) {
			if (ce == CE_BYTES) {
				error("argument entry %"PRIu64
				      " cannot be converted"
				      " from \"bytes\" to \"UTF-8\";"
				      " it contains an invalid byte"
				      " in position %"PRIu64
				      " (\\x%0x)",
				      (uint64_t)i + 1,
				      (uint64_t)err + 1,
				      (unsigned)str[err]);
			} else if (raw) {
				error("argument entry %"PRIu64
				      " is marked as \"UTF-8\""
				      " but contains an invalid byte"
				      " in position %"PRIu64
				      " (\\x%0x)",
				      (uint64_t)i + 1,
				      (uint64_t)err + 1,
				      (unsigned)str[err]);
			} else {
				error("argument entry %"PRIu64
				      " cannot be converted"
				      " from \"%s\" encoding to \"UTF-8\";"
				      " calling 'enc2utf8' on that entry"
				      " returns a string with an invalid"
				      " byte in position %"PRIu64,
				      " (\\x%0x)",
				      (uint64_t)i + 1,
				      encoding_name(ce),
				      (uint64_t)err + 1,
				      (unsigned)str[err]);
			}
		}

		if (!raw || ce == CE_BYTES || ce == CE_NATIVE) {
			if (!duped) {
				PROTECT(ans = duplicate(ans));
				duped = 1;
			}
			SET_STRING_ELT(ans, i,
				       mkCharLenCE((const char *)str,
					           size, CE_UTF8));
		}
	}

	if (duped) {
		UNPROTECT(1);
	}

	return ans;
}


SEXP rutf8_utf8_valid(SEXP sx)
{
	SEXP ans, sstr;
	const uint8_t *str;
	cetype_t ce;
	size_t size, err;
	R_xlen_t i, n;
	int raw, val;;

	if (sx == R_NilValue) {
		return R_NilValue;
	}
	if (!isString(sx)) {
		error("argument is not a character object");
	}

	n = XLENGTH(sx);
	PROTECT(ans = allocVector(LGLSXP, n));
	setAttrib(ans, R_NamesSymbol, getAttrib(sx, R_NamesSymbol));
	setAttrib(ans, R_DimSymbol, getAttrib(sx, R_DimSymbol));
	setAttrib(ans, R_DimNamesSymbol, getAttrib(sx, R_DimNamesSymbol));

	n = XLENGTH(sx);
	for (i = 0; i < n; i++) {
		CHECK_INTERRUPT(i);

		sstr = STRING_ELT(sx, i);
		if (sstr == NA_STRING) {
			LOGICAL(ans)[i] = NA_LOGICAL;
			continue;
		}

		ce = getCharCE(sstr);
		raw = encodes_utf8(ce) || ce == CE_BYTES;

		if (raw) {
			str = (const uint8_t *)CHAR(sstr);
			size = (size_t)XLENGTH(sstr);
		} else {
			str = (const uint8_t *)translate_utf8(sstr);
			size = strlen((const char *)str);
		}

		val = is_valid(str, size, &err) ? TRUE : FALSE;
		LOGICAL(ans)[i] = val;
	}

	UNPROTECT(1);
	return ans;
}


SEXP rutf8_utf8_width(SEXP sx, SEXP squote, SEXP sutf8)
{
	SEXP ans, elt;
	R_xlen_t i, n;
	int quote, utf8, w;

	if (sx == R_NilValue) {
		return R_NilValue;
	}
	if (!isString(sx)) {
		error("argument is not a character object");
	}
	n = XLENGTH(sx);
	quote = LOGICAL(squote)[0] == TRUE;
	utf8 = LOGICAL(sutf8)[0] == TRUE;

	PROTECT(ans = allocVector(INTSXP, n));
	setAttrib(ans, R_NamesSymbol, getAttrib(sx, R_NamesSymbol));
	setAttrib(ans, R_DimSymbol, getAttrib(sx, R_DimSymbol));
	setAttrib(ans, R_DimNamesSymbol, getAttrib(sx, R_DimNamesSymbol));

	for (i = 0; i < n; i++) {
		CHECK_INTERRUPT(i);

		elt = STRING_ELT(sx, i);
		if (elt == NA_STRING) {
			w = NA_INTEGER;
		} else {
			w = charsxp_width(elt, quote, utf8);
		}
		INTEGER(ans)[i] = w;
	}

	UNPROTECT(1);
	return ans;
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
