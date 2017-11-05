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
#include "rutf8.h"

enum cell_type {
	CELL_ENTRY,
	CELL_NA,
	CELL_NAME,
	CELL_ROWNAME
};

struct flags {
	int entry;
	int na;
	int name;
	int rowname;
};

static int flags_get(const struct flags *f, enum cell_type t)
{
	switch (t) {
	case CELL_ENTRY:
		return f->entry;
	case CELL_NA:
		return f->na;
	case CELL_NAME:
		return f->name;
	case CELL_ROWNAME:
		return f->rowname;
	default:
		return -1;
	}
}

static void render_cell(struct utf8lite_render *r, const struct flags *f,
			enum cell_type t, SEXP sx, int right, int pad)
{
	struct rutf8_string str;
	int err = 0, width, quote, old;

	old = utf8lite_render_set_flags(r, flags_get(f, t));

	if (right) {
		TRY(utf8lite_render_spaces(r, pad));
	}

	rutf8_string_init(&str, sx);
	width = 0;
	quote = r->flags & UTF8LITE_ESCAPE_DQUOTE;
	rutf8_string_render(r, &str, width, quote, RUTF8_JUSTIFY_NONE);

	if (!right) {
		TRY(utf8lite_render_spaces(r, pad));
	}
exit:
	CHECK_ERROR(err);
	utf8lite_render_set_flags(r, old);
}


static void render_entry(struct utf8lite_render *r, const struct flags *f,
			 SEXP sx, int right, int pad)
{
	render_cell(r, f, CELL_ENTRY, sx, right, pad);
}


static void render_na(struct utf8lite_render *r, const struct flags *f,
		      SEXP sx, int right, int pad)
{
	render_cell(r, f, CELL_NA, sx, right, pad);
}


static void render_name(struct utf8lite_render *r, const struct flags *f,
			SEXP sx, int right, int pad)
{
	render_cell(r, f, CELL_NAME, sx, right, pad);
}


static void render_rowname(struct utf8lite_render *r, const struct flags *f,
			   SEXP sx, int pad)
{
	render_cell(r, f, CELL_ROWNAME, sx, 0, pad);
}


static int charsxp_width(SEXP sx, int flags)
{
	struct rutf8_string text;
	int quotes = (flags & UTF8LITE_ESCAPE_DQUOTE) ? 2 : 0;
	rutf8_string_init(&text, sx);
	return rutf8_string_width(&text, flags) + quotes;
}


static int render_range(struct utf8lite_render *r, const struct flags *f,
			SEXP sx, SEXP na_print, int begin, int end,
			int print_gap, int right, int max, int namewidth,
			const int *colwidths)
{
	SEXP elt, name, dim_names, row_names, col_names;
	R_xlen_t ix;
	int i, j, nrow, w, nprint, width;
	int err = 0, nprot = 0;

	PROTECT(dim_names = getAttrib(sx, R_DimNamesSymbol)); nprot++;
	row_names = VECTOR_ELT(dim_names, 0);
	col_names = VECTOR_ELT(dim_names, 1);
	nrow = nrows(sx);
	nprint = 0;

	if (col_names != R_NilValue) {
		TRY(utf8lite_render_spaces(r, namewidth));

		for (j = begin; j < end; j++) {
			name = STRING_ELT(col_names, j);
			assert(name != NA_STRING);

			if (j > begin || row_names != R_NilValue) {
				TRY(utf8lite_render_spaces(r, print_gap));
			}
			w = charsxp_width(name, f->name);
			render_name(r, f, name, right, colwidths[j] - w);
		}
		TRY(utf8lite_render_newlines(r, 1));
	}

	for (i = 0; i < nrow; i++) {
		CHECK_INTERRUPT(i);

		if (nprint == max) {
			goto exit;
		}

		if (row_names != R_NilValue) {
			name = STRING_ELT(row_names, i);
			assert(name != NA_STRING);

			w = charsxp_width(name, f->rowname);
			render_rowname(r, f, name, namewidth - w);
		}

		for (j = begin; j < end; j++) {
			if (nprint == max) {
				TRY(utf8lite_render_newlines(r, 1));
				goto exit;
			}
			nprint++;

			if (j > begin || row_names != R_NilValue) {
				TRY(utf8lite_render_spaces(r, print_gap));
			}

			width = colwidths[j];
			ix = (R_xlen_t)i + (R_xlen_t)j * (R_xlen_t)nrow;
			elt = STRING_ELT(sx, ix);
			if (elt == NA_STRING) {
				w = charsxp_width(na_print, f->na);
				render_na(r, f, na_print, right, width - w);
			} else {
				w = charsxp_width(elt, f->entry);
				render_entry(r, f, elt, right, width - w);
			}
		}

		TRY(utf8lite_render_newlines(r, 1));
	}
exit:
	UNPROTECT(nprot);
	CHECK_ERROR(err);
	return nprint;
}


SEXP rutf8_render_table(SEXP sx, SEXP swidth, SEXP squote, SEXP sna_print,
			SEXP sprint_gap, SEXP sright, SEXP smax,
			SEXP sdisplay, SEXP slinewidth, SEXP sutf8)
{
	SEXP ans, na_print, str, srender, elt, dim_names, row_names, col_names;
	struct utf8lite_render *render;
	struct flags flags;
	R_xlen_t ix, nx;
	int i, j, nrow, ncol;
	int width, quote, print_gap, right, max, display, linewidth, utf8;
	int begin, end, w, nprint, lw, namewidth, *colwidths;
	int nprot = 0;

	PROTECT(dim_names = getAttrib(sx, R_DimNamesSymbol)); nprot++;
	row_names = VECTOR_ELT(dim_names, 0);
	col_names = VECTOR_ELT(dim_names, 1);
	nrow = nrows(sx);
	ncol = ncols(sx);
	nx = XLENGTH(sx);

	width = INTEGER(swidth)[0];
        quote = LOGICAL(squote)[0] == TRUE;
	PROTECT(na_print = STRING_ELT(sna_print, 0)); nprot++;
	print_gap = INTEGER(sprint_gap)[0];
	right = LOGICAL(sright)[0] == TRUE;
	max = INTEGER(smax)[0];
	display = LOGICAL(sdisplay)[0] == TRUE;
	linewidth = INTEGER(slinewidth)[0];
	utf8 = LOGICAL(sutf8)[0] == TRUE;

	flags.entry = (UTF8LITE_ESCAPE_CONTROL | UTF8LITE_ENCODE_C);
	if (quote) {
		flags.entry |= UTF8LITE_ESCAPE_DQUOTE;
	}
        if (display) {
                flags.entry |= UTF8LITE_ENCODE_RMDI;
                flags.entry |= UTF8LITE_ENCODE_EMOJIZWSP;
        }
        if (!utf8) {
                flags.entry |= UTF8LITE_ESCAPE_UTF8;
        }
#if defined(_WIN32) || defined(_WIN64)
        flags.entry |= UTF8LITE_ESCAPE_EXTENDED;
#endif
	flags.na = flags.entry & ~UTF8LITE_ESCAPE_DQUOTE;
	flags.name = flags.na;
	flags.rowname = flags.name;

	PROTECT(srender = rutf8_alloc_render(0)); nprot++;
	render = rutf8_as_render(srender);

	namewidth = 0;
	if (row_names == R_NilValue) {
		namewidth = 0;
	} else {
		for (i = 0; i < nrow; i++) {
			CHECK_INTERRUPT(i);

			elt = STRING_ELT(row_names, i);
			assert(elt != NA_STRING);

			w = charsxp_width(elt, flags.rowname);
			if (w > namewidth) {
				namewidth = w;
			}
		}
	}

	if (ncol == 0) {
		nprint = render_range(render, &flags, sx, na_print, 0, 0,
				      print_gap, right, max, namewidth, NULL);
		goto exit;
	}

	colwidths = (void *)R_alloc(ncol, sizeof(*colwidths));
	for (j = 0; j < ncol; j++) {
		colwidths[j] = width;
	}
	if (col_names != R_NilValue) {
		for (j = 0; j < ncol; j++) {
			elt = STRING_ELT(col_names, j);
			assert(elt != NA_STRING);
			w = charsxp_width(elt, flags.name);
			if (w > colwidths[j]) {
				colwidths[j] = w;
			}
		}
	}

	j = 0;
	for (ix = 0; ix < nx; ix++) {
		elt = STRING_ELT(sx, ix);

		if (elt == NA_STRING) {
			w = charsxp_width(na_print, flags.na);
		} else {
			w = charsxp_width(elt, flags.entry);
		}

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
		lw = namewidth;
		end = begin;

		while (end != ncol) {
			// break if including the column puts us over the
			// width; we do the calculations like this to
			// avoid integer overflow

			if (end > begin || row_names != R_NilValue) {
				if (lw > linewidth - print_gap) {
					break;
				}
				lw += print_gap;
			}

			if (lw > linewidth - colwidths[end]) {
				break;
			}
			lw += colwidths[end];

			end++;
		}

		if (begin == end) {
			// include at least one column, even if it
			// puts us over the line width
			end++;
		}

		nprint += render_range(render, &flags, sx, na_print, begin,
				       end, print_gap, right, max - nprint,
				       namewidth, colwidths);
		begin = end;
	}

exit:
	PROTECT(str = mkCharLenCE(render->string, render->length, CE_UTF8));
	nprot++;
	PROTECT(ans = ScalarString(str)); nprot++;

	rutf8_free_render(srender);
	UNPROTECT(nprot);
	return ans;
}
