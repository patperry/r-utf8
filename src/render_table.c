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

struct style {
	struct flags flags;
	const char *names;
	int names_len;
	const char *rownames;
	int rownames_len;
	int right;
	const char *esc_open;
	const char *esc_close;
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


static const char *sgr_get(const struct style *s, enum cell_type t, int *lenptr)
{
	switch (t) {
	case CELL_NAME:
		*lenptr = s->names_len;
		return s->names;

	case CELL_ROWNAME:
		*lenptr = s->rownames_len;
		return s->rownames;

	default:
		*lenptr = 0;
		return NULL;
	}
}


static int charsxp_width(SEXP sx, int flags)
{
	struct rutf8_string text;
	int quotes = (flags & UTF8LITE_ESCAPE_DQUOTE) ? 2 : 0;
	rutf8_string_init(&text, sx);
	return rutf8_string_width(&text, flags) + quotes;
}


static void render_cell(struct utf8lite_render *r, const struct style *s,
			enum cell_type t, SEXP sx, int width)
{
	struct rutf8_string str;
	const char *sgr;
	int err = 0, w, pad, right, quote, old, nsgr;

	old = r->flags;
	TRY(utf8lite_render_set_flags(r, flags_get(&s->flags, t)));
	quote = r->flags & UTF8LITE_ESCAPE_DQUOTE;
	right = (t == CELL_ROWNAME) ? 0 : s->right;
	sgr = sgr_get(s, t, &nsgr);

	w = charsxp_width(sx, r->flags);
	pad = width - w;

	if (sgr) {
		TRY(utf8lite_render_raw(r, sgr, nsgr));
	}

	if (right) {
		TRY(utf8lite_render_chars(r, ' ', pad));
	}

	if (t == CELL_ENTRY) {
		TRY(utf8lite_render_set_style(r, s->esc_open, s->esc_close));
	}

	rutf8_string_init(&str, sx);
	rutf8_string_render(r, &str, 0, quote, RUTF8_JUSTIFY_NONE);

	if (t == CELL_ENTRY) {
		TRY(utf8lite_render_set_style(r, NULL, NULL));
	}

	if (!right) {
		TRY(utf8lite_render_chars(r, ' ', pad));
	}

	if (sgr) {
		TRY(utf8lite_render_raw(r, RUTF8_STYLE_CLOSE,
					RUTF8_STYLE_CLOSE_SIZE));
	}
	TRY(utf8lite_render_set_flags(r, old));
exit:
	CHECK_ERROR(err);
}


static void render_entry(struct utf8lite_render *r, const struct style *s,
			 SEXP sx, int width)
{
	render_cell(r, s, CELL_ENTRY, sx, width);
}


static void render_na(struct utf8lite_render *r, const struct style *s,
		      SEXP sx, int width)
{
	render_cell(r, s, CELL_NA, sx, width);
}


static void render_name(struct utf8lite_render *r, const struct style *s,
			SEXP sx, int width)
{
	render_cell(r, s, CELL_NAME, sx, width);
}


static void render_rowname(struct utf8lite_render *r, const struct style *s,
			   SEXP sx, int width)
{
	render_cell(r, s, CELL_ROWNAME, sx, width);
}


static int render_range(struct utf8lite_render *r, const struct style *s,
			SEXP sx, SEXP na_print, int begin, int end,
			int print_gap, int max, int namewidth,
			const int *colwidths)
{
	SEXP elt, name, dim_names, row_names, col_names;
	R_xlen_t ix;
	int i, j, nrow, nprint, width;
	int err = 0, nprot = 0;

	PROTECT(dim_names = getAttrib(sx, R_DimNamesSymbol)); nprot++;
	row_names = VECTOR_ELT(dim_names, 0);
	col_names = VECTOR_ELT(dim_names, 1);
	nrow = nrows(sx);
	nprint = 0;

	if (col_names != R_NilValue) {
		TRY(utf8lite_render_chars(r, ' ', namewidth));

		for (j = begin; j < end; j++) {
			PROTECT(name = STRING_ELT(col_names, j)); nprot++;
			assert(name != NA_STRING);

			if (j > begin || row_names != R_NilValue) {
				TRY(utf8lite_render_chars(r, ' ', print_gap));
			}
			render_name(r, s, name, colwidths[j]);
			UNPROTECT(1); nprot--;
		}
		TRY(utf8lite_render_newlines(r, 1));
	}

	for (i = 0; i < nrow; i++) {
		CHECK_INTERRUPT(i);

		if (nprint == max) {
			goto exit;
		}

		if (row_names != R_NilValue) {
			PROTECT(name = STRING_ELT(row_names, i)); nprot++;
			assert(name != NA_STRING);
			render_rowname(r, s, name, namewidth);
			UNPROTECT(1); nprot--;
		}

		for (j = begin; j < end; j++) {
			if (nprint == max) {
				TRY(utf8lite_render_newlines(r, 1));
				goto exit;
			}
			nprint++;

			if (j > begin || row_names != R_NilValue) {
				TRY(utf8lite_render_chars(r, ' ', print_gap));
			}

			width = colwidths[j];
			ix = (R_xlen_t)i + (R_xlen_t)j * (R_xlen_t)nrow;
			PROTECT(elt = STRING_ELT(sx, ix)); nprot++;
			if (elt == NA_STRING) {
				render_na(r, s, na_print, width);
			} else {
				render_entry(r, s, elt, width);
			}
			UNPROTECT(1); nprot--;
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
			SEXP snames, SEXP srownames, SEXP sescapes,
			SEXP sdisplay, SEXP sstyle, SEXP sutf8,
			SEXP slinewidth)
{
	SEXP ans, na_print, str, srender, elt, dim_names, row_names, col_names;
	struct utf8lite_render *render;
	struct style s;
	R_xlen_t ix, nx;
	int i, j, nrow, ncol;
	int width, quote, print_gap, max, display, style, linewidth, utf8;
	int begin, end, w, nprint, lw, namewidth, *colwidths;
	int nprot = 0;

	memset(&s, 0, sizeof(s));

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
	s.right = LOGICAL(sright)[0] == TRUE;
	max = INTEGER(smax)[0];
	display = LOGICAL(sdisplay)[0] == TRUE;
	style = LOGICAL(sstyle)[0] == TRUE;
	utf8 = LOGICAL(sutf8)[0] == TRUE;
	linewidth = INTEGER(slinewidth)[0];

	s.flags.entry = (UTF8LITE_ESCAPE_CONTROL | UTF8LITE_ENCODE_C);
	if (quote) {
		s.flags.entry |= UTF8LITE_ESCAPE_DQUOTE;
	}
        if (display) {
                s.flags.entry |= UTF8LITE_ENCODE_RMDI;
                s.flags.entry |= UTF8LITE_ENCODE_EMOJIZWSP;
        }
	if (style) {
		if ((s.names = rutf8_as_style(snames))) {
			s.names_len = (int)strlen(s.names);
		}
		if ((s.rownames = rutf8_as_style(srownames))) {
			s.rownames_len = (int)strlen(s.rownames);
		}
	}
        if (!utf8) {
                s.flags.entry |= UTF8LITE_ESCAPE_UTF8;
        }
#if defined(_WIN32) || defined(_WIN64)
        s.flags.entry |= UTF8LITE_ESCAPE_EXTENDED;
#endif
	s.flags.na = s.flags.entry & ~UTF8LITE_ESCAPE_DQUOTE;
	s.flags.name = s.flags.na;
	s.flags.rowname = s.flags.name;

	PROTECT(srender = rutf8_alloc_render(0)); nprot++;
	render = rutf8_as_render(srender);

	if (style) {
		if ((s.esc_open = rutf8_as_style(sescapes))) {
			s.esc_close = RUTF8_STYLE_CLOSE;
		}
	}

	namewidth = 0;
	if (row_names == R_NilValue) {
		namewidth = 0;
	} else {
		for (i = 0; i < nrow; i++) {
			CHECK_INTERRUPT(i);

			PROTECT(elt = STRING_ELT(row_names, i)); nprot++;
			assert(elt != NA_STRING);

			w = charsxp_width(elt, s.flags.rowname);
			if (w > namewidth) {
				namewidth = w;
			}

			UNPROTECT(1); nprot--;
		}
	}

	if (ncol == 0) {
		nprint = render_range(render, &s, sx, na_print, 0, 0,
				      print_gap, max, namewidth, NULL);
		goto exit;
	}

	colwidths = (void *)R_alloc(ncol, sizeof(*colwidths));
	for (j = 0; j < ncol; j++) {
		colwidths[j] = width;
	}
	if (col_names != R_NilValue) {
		for (j = 0; j < ncol; j++) {
			PROTECT(elt = STRING_ELT(col_names, j)); nprot++;
			assert(elt != NA_STRING);
			w = charsxp_width(elt, s.flags.name);
			if (w > colwidths[j]) {
				colwidths[j] = w;
			}
			UNPROTECT(1); nprot--;
		}
	}

	j = 0;
	for (ix = 0; ix < nx; ix++) {
		PROTECT(elt = STRING_ELT(sx, ix)); nprot++;

		if (elt == NA_STRING) {
			w = charsxp_width(na_print, s.flags.na);
		} else {
			w = charsxp_width(elt, s.flags.entry);
		}

		if (w > colwidths[j]) {
			colwidths[j] = w;
		}

		if ((ix + 1) % nrow == 0) {
			j++;
		}

		UNPROTECT(1); nprot--;
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

		nprint += render_range(render, &s, sx, na_print, begin,
				       end, print_gap, max - nprint,
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
