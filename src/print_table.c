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
#include <stddef.h>
#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <R_ext/Riconv.h>

#include "rutf8.h"

#if (defined(_WIN32) || defined(_WIN64))
#include <windows.h>

#define Win32
#include <R_ext/RStartup.h>
extern UImode CharacterMode;
extern unsigned int localeCP;

static const char *translate(SEXP charsxp, int is_stdout)
{
	const char *raw;
	LPWSTR wstr;
	char *str;
	int wlen, len, n;
	cetype_t ce;
	UINT cp;

	raw = CHAR(charsxp);
	n = XLENGTH(charsxp);
	ce = getCharCE(charsxp);

	if (ce == CE_ANY) {
		return raw; // no need to translate ASCII
	}

	// mimic the WinCheckUTF8 behavior in base R main/connections.c
	if (CharacterMode == RGui && is_stdout) {
		if (n > INT_MAX - 7) {
			error("string length exceeds maximum (%d)", INT_MAX);
		}
		str = R_alloc(n + 7, 1);

		// mimic the EncodeString behavior in main/printutils.c
		//   prepend UTF8in  (02 FF FE)
		//   append  UTF8out (03 FF FE)
		memcpy(str, "\x02\xFF\xFE", 3);
		memcpy(str + 3, raw, n);
		memcpy(str + 3 + n, "\x03\xFF\xFE", 4); // include NUL
		return str;
	}

	// convert from UTF-8 to UTF-16
	// use n + 1 for length to include trailing NUL
	wlen = MultiByteToWideChar(CP_UTF8, 0, raw, n + 1, NULL, 0);
	wstr = (LPWSTR)R_alloc(wlen, sizeof(*wstr));
	MultiByteToWideChar(CP_UTF8, 0, raw, n + 1, wstr, wlen);

	// for stdout, use native code page; otherwise use locale page
	cp = is_stdout ? 0 : (UINT)localeCP;
	if (cp == 0) {
		// Use ConsoleCP for terminal output to stdout, ACP otherwise
		// (see https://go-review.googlesource.com/c/27575/ )
		cp = (CharacterMode == RTerm) ?  GetConsoleCP() : GetACP();
	}

	// convert from UTF-16 to native code page
	len = WideCharToMultiByte(cp, 0, wstr, wlen, NULL, 0, NULL, NULL);
	str = R_alloc(len, 1);
	WideCharToMultiByte(cp, 0, wstr, wlen, str, len, NULL, NULL);

	return str;
}
#else
#define translate(x, is_stdout) translateChar(x)
#endif


#define NEEDS(n) \
	do { \
		if ((n) && (nbuf > nbuf_max - (n))) { \
			nbuf_max0 = nbuf_max; \
			array_size_add(&nbuf_max, 1, nbuf, (n)); \
			buf = S_realloc(buf, nbuf_max, nbuf_max0, 1); \
		} \
	} while (0)

#define PRINT_SPACES(n) \
	do { \
		NEEDS(n); \
		if ((n) > 0) { \
			memset(buf + nbuf, ' ', (n)); \
			nbuf += (n); \
		} \
	} while (0)

#define PRINT_CHAR(ch) \
	do { \
		NEEDS(1); \
		buf[nbuf] = (ch); \
		nbuf++; \
	} while (0)

#define PRINT_STRING(str, n) \
	do { \
		NEEDS(n); \
		memcpy(buf + nbuf, str, (n)); \
		nbuf += (n); \
	} while (0)

#define PRINT_ENTRY(str, n, pad) \
	do { \
		if (right) PRINT_SPACES(pad); \
		PRINT_STRING(str, n); \
		if (!right) PRINT_SPACES(pad); \
	} while (0)

#define FLUSH() \
	do { \
		PRINT_CHAR('\0'); \
		Rprintf("%s", buf); \
		nbuf = 0; \
	} while (0)

static int print_range(struct utf8lite_render *render, SEXP sx, int begin,
		       int end, int print_gap, int right, int max,
		       int is_stdout, int namewidth, const int *colwidths)
{
	SEXP elt, name, dim_names, row_names, col_names;
	R_xlen_t ix;
	const char *str;
	char *buf;
	int nbuf, nbuf_max, nbuf_max0;
	int i, j, nrow, n, w, nprint, width, utf8;
	int nprot = 0;

	PROTECT(dim_names = getAttrib(sx, R_DimNamesSymbol)); nprot++;
	row_names = VECTOR_ELT(dim_names, 0);
	col_names = VECTOR_ELT(dim_names, 1);
	nrow = nrows(sx);
	nprint = 0;
	utf8 = 1;

	nbuf = 0;
	nbuf_max = 128;
	buf = R_alloc(nbuf_max + 1, 1);

	if (col_names != R_NilValue) {
		PRINT_SPACES(namewidth);

		for (j = begin; j < end; j++) {
			name = STRING_ELT(col_names, j);
			assert(name != NA_STRING);

			str = translate(name, is_stdout);
			w = charsxp_width(name, 0, utf8);
			n = strlen(str);

			if (j > begin || row_names != R_NilValue) {
				PRINT_SPACES(print_gap);
			}
			PRINT_ENTRY(str, n, colwidths[j] - w);
		}
		PRINT_CHAR('\n');
		FLUSH();
	}

	for (i = 0; i < nrow; i++) {
		CHECK_INTERRUPT(i);

		if (nprint == max) {
			FLUSH();
			goto out;
		}

		if (row_names != R_NilValue) {
			name = STRING_ELT(row_names, i);
			assert(name != NA_STRING);

			str = translate(name, is_stdout);
			w = charsxp_width(name, 0, utf8);
			n = strlen(str);

			PRINT_STRING(str, n);
			PRINT_SPACES(namewidth - w);
		}

		for (j = begin; j < end; j++) {
			if (nprint == max) {
				PRINT_CHAR('\n');
				FLUSH();
				goto out;
			}
			nprint++;

			width = colwidths[j];
			ix = (R_xlen_t)i + (R_xlen_t)j * (R_xlen_t)nrow;
			elt = STRING_ELT(sx, ix);

			if (j > begin || row_names != R_NilValue) {
				PRINT_SPACES(print_gap);
			}

			str = translate(elt, is_stdout);
			w = charsxp_width(elt, 0, utf8);
			n = strlen(str);
			PRINT_ENTRY(str, n, width - w);
		}

		PRINT_CHAR('\n');
		FLUSH();
	}

	(void)is_stdout; // unused unless on Windows
out:
	UNPROTECT(nprot);
	return nprint;
}


SEXP rutf8_print_table(SEXP sx, SEXP sprint_gap, SEXP sright, SEXP smax,
		       SEXP swidth, SEXP sis_stdout)
{
	SEXP srender, elt, dim_names, row_names, col_names;
	struct utf8lite_render *render;
	R_xlen_t ix, nx;
	int i, j, nrow, ncol;
	int print_gap, right, max, width, is_stdout, utf8;
	int begin, end, w, nprint, linewidth, namewidth, *colwidths;
	int nprot = 0;

	PROTECT(srender = rutf8_alloc_render(0)); nprot++;
	render = rutf8_as_render(srender);

	PROTECT(dim_names = getAttrib(sx, R_DimNamesSymbol)); nprot++;
	row_names = VECTOR_ELT(dim_names, 0);
	col_names = VECTOR_ELT(dim_names, 1);
	nrow = nrows(sx);
	ncol = ncols(sx);
	nx = XLENGTH(sx);
	utf8 = 1;

	print_gap = INTEGER(sprint_gap)[0];
	right = LOGICAL(sright)[0] == TRUE;
	width = INTEGER(swidth)[0];
	max = INTEGER(smax)[0];
	is_stdout = LOGICAL(sis_stdout)[0] == TRUE;

	namewidth = 0;
	if (row_names == R_NilValue) {
		namewidth = 0;
	} else {
		for (i = 0; i < nrow; i++) {
			CHECK_INTERRUPT(i);

			elt = STRING_ELT(row_names, i);
			assert(elt != NA_STRING);

			w = charsxp_width(elt, 0, utf8);
			if (w > namewidth) {
				namewidth = w;
			}
		}
	}

	if (ncol == 0) {
		nprint = print_range(render, sx, 0, 0, print_gap, right, max,
				     is_stdout, namewidth, NULL);
		goto exit;
	}

	colwidths = (void *)R_alloc(ncol, sizeof(*colwidths));
	memset(colwidths, 0, ncol * sizeof(*colwidths));
	if (col_names != R_NilValue) {
		for (j = 0; j < ncol; j++) {
			elt = STRING_ELT(col_names, j);
			assert(elt != NA_STRING);
			colwidths[j] = charsxp_width(elt, 0, utf8);
		}
	}

	j = 0;
	for (ix = 0; ix < nx; ix++) {
		elt = STRING_ELT(sx, ix);
		assert(elt != NA_STRING);

		w = charsxp_width(elt, 0, utf8);
		if (w > colwidths[j]) {
			colwidths[j] = w;
		}

		if ((ix + 1) % nrow == 0) {
			j++;
		}
	}

	nprint = 0;
	begin = 0;
	while (begin != ncol) {
		linewidth = namewidth;
		end = begin;

		while (end != ncol) {
			// break if including the column puts us over the
			// width; we do the calculations like this to
			// avoid integer overflow

			if (end > begin || row_names != R_NilValue) {
				if (linewidth > width - print_gap) {
					break;
				}
				linewidth += print_gap;
			}

			if (linewidth > width - colwidths[end]) {
				break;
			}
			linewidth += colwidths[end];

			end++;
		}

		if (begin == end) {
			// include at least one column, even if it
			// puts us over the width
			end++;
		}

		nprint += print_range(render, sx, begin, end, print_gap, right,
				      max - nprint, is_stdout, namewidth,
				      colwidths);
		begin = end;
	}
exit:
	rutf8_free_render(srender);
	UNPROTECT(nprot);
	return ScalarInteger(nprint);
}
