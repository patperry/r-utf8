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


const char *rutf8_as_style(SEXP style)
{
	SEXP elt;
	char *ans;
	int n;

	if (style == R_NilValue) {
		return NULL;
	}

	PROTECT(elt = STRING_ELT(style, 0));

	n = LENGTH(elt);
	ans = R_alloc(2 + n + 2, 1);

	ans[0] = '\033';
	ans[1] = '[';
	memcpy(ans + 2, CHAR(elt), n);
	ans[2 + n] = 'm';
	ans[3 + n] = '\0';

	UNPROTECT(1);
	return ans;
}


int rutf8_encodes_utf8(cetype_t ce)
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


#if (defined(_WIN32) || defined(_WIN64))
#include <windows.h>
extern unsigned int localeCP;

const char *rutf8_translate_utf8(SEXP x)
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

const char *rutf8_translate_utf8(SEXP x)
{
	return translateCharUTF8(x);
}

#endif
