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


static void render_entry(struct utf8lite_render *r, SEXP sx, int right,
			 int pad)
{
	int err = 0;

	if (right) {
		TRY(utf8lite_render_spaces(r, pad));
	}

	TRY(utf8lite_render_bytes(r, CHAR(sx), XLENGTH(sx)));

	if (!right) {
		TRY(utf8lite_render_spaces(r, pad));
	}
exit:
	CHECK_ERROR(err);
}


static int render_range(struct utf8lite_render *r, SEXP sx, int begin,
			int end, int print_gap, int right, int max,
			int namewidth, const int *colwidths)
{
	SEXP elt, name, dim_names, row_names, col_names;
	R_xlen_t ix;
	int i, j, nrow, w, nprint, width, utf8;
	int err = 0, nprot = 0;

	PROTECT(dim_names = getAttrib(sx, R_DimNamesSymbol)); nprot++;
	row_names = VECTOR_ELT(dim_names, 0);
	col_names = VECTOR_ELT(dim_names, 1);
	nrow = nrows(sx);
	nprint = 0;
	utf8 = 1;

	if (col_names != R_NilValue) {
		TRY(utf8lite_render_spaces(r, namewidth));

		for (j = begin; j < end; j++) {
			name = STRING_ELT(col_names, j);
			assert(name != NA_STRING);

			if (j > begin || row_names != R_NilValue) {
				TRY(utf8lite_render_spaces(r, print_gap));
			}
			w = charsxp_width(name, 0, utf8);
			render_entry(r, name, right, colwidths[j] - w);
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

			w = charsxp_width(name, 0, utf8);
			render_entry(r, name, 0, namewidth - w);
		}

		for (j = begin; j < end; j++) {
			if (nprint == max) {
				TRY(utf8lite_render_newlines(r, 1));
				goto exit;
			}
			nprint++;

			width = colwidths[j];
			ix = (R_xlen_t)i + (R_xlen_t)j * (R_xlen_t)nrow;
			elt = STRING_ELT(sx, ix);

			if (j > begin || row_names != R_NilValue) {
				TRY(utf8lite_render_spaces(r, print_gap));
			}

			w = charsxp_width(elt, 0, utf8);
			render_entry(r, elt, right, width - w);
		}

		TRY(utf8lite_render_newlines(r, 1));
	}
exit:
	UNPROTECT(nprot);
	CHECK_ERROR(err);
	return nprint;
}


SEXP rutf8_render_table(SEXP sx, SEXP sprint_gap, SEXP sright, SEXP smax,
			SEXP swidth)
{
	SEXP srender, elt, dim_names, row_names, col_names;
	struct utf8lite_render *render;
	R_xlen_t ix, nx;
	int i, j, nrow, ncol;
	int print_gap, right, max, width, utf8;
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
		nprint = render_range(render, sx, 0, 0, print_gap, right,
				      max, namewidth, NULL);
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

		nprint += render_range(render, sx, begin, end, print_gap,
				       right, max - nprint, namewidth,
				       colwidths);
		begin = end;
	}
exit:
	Rprintf("%s", render->string);
	rutf8_free_render(srender);
	UNPROTECT(nprot);
	return ScalarInteger(nprint);
}
