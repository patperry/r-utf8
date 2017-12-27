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
#include "rutf8.h"


int rutf8_text_width(const struct utf8lite_text *text, int flags)
{
	struct utf8lite_graphscan scan;
	int err = 0, width, w;

	utf8lite_graphscan_make(&scan, text);
	width = 0;
	while (utf8lite_graphscan_advance(&scan)) {
		TRY(utf8lite_graph_measure(&scan.current, flags, &w));
		if (w < 0) {
			return -1;
		}
		if (width > INT_MAX - w) {
			Rf_error("width exceeds maximum (%d)", INT_MAX);
		}
		width += w;
	}
exit:
	CHECK_ERROR(err);
	return width;
}


int rutf8_text_lwidth(const struct utf8lite_text *text, int flags,
		      int limit, int ellipsis)
{
	struct utf8lite_graphscan scan;
	int err = 0, width, w;

	utf8lite_graphscan_make(&scan, text);
	width = 0;
	while (utf8lite_graphscan_advance(&scan)) {
		TRY(utf8lite_graph_measure(&scan.current, flags, &w));
		if (w < 0) {
			return -1;
		}
		if (width > limit - w) {
			return width + ellipsis;
		}
		width += w;
	}
exit:
	CHECK_ERROR(err);
	return width;
}


int rutf8_text_rwidth(const struct utf8lite_text *text, int flags,
		      int limit, int ellipsis)
{
	struct utf8lite_graphscan scan;
	int err = 0, width, w;

	utf8lite_graphscan_make(&scan, text);
	utf8lite_graphscan_skip(&scan);
	width = 0;
	while (utf8lite_graphscan_retreat(&scan)) {
		TRY(utf8lite_graph_measure(&scan.current, flags, &w));
		if (w < 0) {
			return -1;
		}
		if (width > limit - w) {
			return width + ellipsis;
		}
		width += w;
	}
exit:
	return width;
}


static void rutf8_text_lrender(struct utf8lite_render *r,
			       const struct utf8lite_text *text,
			       int width_min, int quote, int centre)
{
	struct utf8lite_graphscan scan;
	int err = 0, w, fullwidth, width, quotes;

	assert(width_min >= 0);

	quotes = quote ? 2 : 0;
	width = 0;

	if (centre && width_min > 0) {
		fullwidth = rutf8_text_width(text, r->flags);
		// ensure fullwidth + quotes doesn't overflow
		if (fullwidth <= width_min - quotes) {
			width = (width_min - fullwidth - quotes) / 2;
			TRY(utf8lite_render_chars(r, ' ', width));
		}
	}

	if (quote) {
		TRY(utf8lite_render_raw(r, "\"", 1));
		assert(width < INT_MAX); // width <= width_min / 2
		width++;
	}

	utf8lite_graphscan_make(&scan, text);
	while (utf8lite_graphscan_advance(&scan)) {
		TRY(utf8lite_graph_measure(&scan.current, r->flags, &w));
		TRY(utf8lite_render_graph(r, &scan.current));

		assert(w >= 0);
		if (width <= width_min - w) {
			width += w;
		} else {
			width = width_min; // truncate to avoid overflow
		}
	}

	if (quote) {
		TRY(utf8lite_render_raw(r, "\"", 1));
		if (width < width_min) { // avoid overflow
			width++;
		}
	}

	TRY(utf8lite_render_chars(r, ' ', width_min - width));
exit:
	CHECK_ERROR(err);
}


static void rutf8_text_rrender(struct utf8lite_render *r,
			       const struct utf8lite_text *text,
			       int width_min, int quote)
{
	struct utf8lite_graphscan scan;
	int err = 0, fullwidth, quotes;

	quotes = quote ? 2 : 0;

	if (width_min > 0) {
		fullwidth = rutf8_text_width(text, r->flags);
		// ensure fullwidth + quotes doesn't overflow
		if (fullwidth <= width_min - quotes) {
			fullwidth += quotes;
			TRY(utf8lite_render_chars(r, ' ',
						  width_min - fullwidth));
		}
	}

	if (quote) {
		TRY(utf8lite_render_raw(r, "\"", 1));
	}

	utf8lite_graphscan_make(&scan, text);
	while (utf8lite_graphscan_advance(&scan)) {
		TRY(utf8lite_render_graph(r, &scan.current));
	}

	if (quote) {
		TRY(utf8lite_render_raw(r, "\"", 1));
	}
exit:
	CHECK_ERROR(err);
}


void rutf8_text_render(struct utf8lite_render *r,
		       const struct utf8lite_text *text,
		       int width, int quote, enum rutf8_justify_type justify)
{
	int centre;
	if (justify == RUTF8_JUSTIFY_RIGHT) {
		rutf8_text_rrender(r, text, width, quote);
	} else {
		centre = (justify == RUTF8_JUSTIFY_CENTRE);
		rutf8_text_lrender(r, text, width, quote, centre);
	}
}

static SEXP rutf8_text_lformat(struct utf8lite_render *r,
			       const struct utf8lite_text *text,
			       int trim, int chars, int quote,
			       const char *ellipsis, size_t nellipsis,
			       int wellipsis, int flags, int width_max,
			       int centre)
{
	SEXP ans = R_NilValue;
	struct utf8lite_graphscan scan;
	int err = 0, w, trunc, bfill, efill, fullwidth, width, quotes;

	quotes = quote ? 2 : 0;

	bfill = 0;
	if (centre && !trim) {
		fullwidth = (rutf8_text_lwidth(text, flags, chars, wellipsis)
			     + quotes);
		if (fullwidth < width_max) {
			bfill = (width_max - fullwidth) / 2;
			TRY(utf8lite_render_chars(r, ' ', bfill));
		}
	}

	width = 0;
	trunc = 0;
	utf8lite_graphscan_make(&scan, text);

	while (!trunc && utf8lite_graphscan_advance(&scan)) {
		TRY(utf8lite_graph_measure(&scan.current, flags, &w));

		if (width > chars - w) {
			w = wellipsis;
			TRY(utf8lite_render_raw(r, ellipsis, nellipsis));
			trunc = 1;
		} else {
			TRY(utf8lite_render_graph(r, &scan.current));
		}

		width += w;
	}

	if (!trim) {
		efill = width_max - width - quotes - bfill;
		TRY(utf8lite_render_chars(r, ' ', efill));
	}

	ans = mkCharLenCE((char *)r->string, r->length, CE_UTF8);
	utf8lite_render_clear(r);
exit:
	CHECK_ERROR(err);
	return ans;
}


static SEXP rutf8_text_rformat(struct utf8lite_render *r,
			       const struct utf8lite_text *text,
			       int trim, int chars, int quote,
			       const char *ellipsis, size_t nellipsis,
			       int wellipsis, int flags, int width_max)
{
	SEXP ans = R_NilValue;
	struct utf8lite_graphscan scan;
	int err = 0, w, width, quotes, trunc;

	quotes = quote ? 2 : 0;

	utf8lite_graphscan_make(&scan, text);
	utf8lite_graphscan_skip(&scan);
	width = 0;
	trunc = 0;

	while (!trunc && utf8lite_graphscan_retreat(&scan)) {
		TRY(utf8lite_graph_measure(&scan.current, flags, &w));

		if (width > chars - w) {
			w = wellipsis;
			trunc = 1;
		}

		width += w;
	}

	if (!trim) {
		TRY(utf8lite_render_chars(r, ' ', width_max - width - quotes));
	}

	if (trunc) {
		TRY(utf8lite_render_raw(r, ellipsis, nellipsis));
	}

	while (utf8lite_graphscan_advance(&scan)) {
		TRY(utf8lite_render_graph(r, &scan.current));
	}

	ans = mkCharLenCE((char *)r->string, r->length, CE_UTF8);
	utf8lite_render_clear(r);
exit:
	CHECK_ERROR(err);
	return ans;
}


SEXP rutf8_text_format(struct utf8lite_render *r,
		       const struct utf8lite_text *text,
		       int trim, int chars, enum rutf8_justify_type justify,
		       int quote, const char *ellipsis, size_t nellipsis,
		       int wellipsis, int flags, int width_max)
{
	int centre;

	if (justify == RUTF8_JUSTIFY_RIGHT) {
		return rutf8_text_rformat(r, text, trim, chars, quote,
					  ellipsis, nellipsis, wellipsis,
					  flags, width_max);
	} else {
		centre = (justify == RUTF8_JUSTIFY_CENTRE);
		return rutf8_text_lformat(r, text, trim, chars, quote,
					  ellipsis, nellipsis, wellipsis,
					  flags, width_max, centre);
	}
}
