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

#include "rutf8.h"


int rutf8_text_width(const struct utf8lite_text *text, int limit,
		     int ellipsis, int flags)
{
	struct utf8lite_graphscan scan;
	int err = 0, width, w;

	utf8lite_graphscan_make(&scan, text);
	width = 0;
	while (utf8lite_graphscan_advance(&scan)) {
		TRY(utf8lite_graph_measure(&scan.current, flags, &w));
		if (width > limit - w) {
			return width + ellipsis;
		}
		width += w;
	}

exit:
	CHECK_ERROR(err);
	return width;
}


int rutf8_text_rwidth(const struct utf8lite_text *text, int limit,
		      int ellipsis, int flags)
{
	struct utf8lite_graphscan scan;
	int err = 0, width, w;

	utf8lite_graphscan_make(&scan, text);
	utf8lite_graphscan_skip(&scan);
	width = 0;
	while (utf8lite_graphscan_retreat(&scan)) {
		TRY(utf8lite_graph_measure(&scan.current, flags, &w));
		if (width > limit - w) {
			return width + ellipsis;
		}
		width += w;
	}

exit:
	return width;
}


SEXP rutf8_text_format(struct utf8lite_render *r,
		       const struct utf8lite_text *text,
		       int trim, int chars, int width_max,
		       int quote, int utf8, int flags, int centre)
{
	SEXP ans = R_NilValue;
	struct utf8lite_graphscan scan;
	const char *ellipsis_str;
	int err = 0, w, trunc, bfill, fullwidth, width, quotes, ellipsis;

	quotes = quote ? 2 : 0;
	ellipsis = utf8 ? 1 : 3;
	ellipsis_str = utf8 ? RUTF8_ELLIPSIS : "...";

	bfill = 0;
	if (centre && !trim) {
		fullwidth = (rutf8_text_width(text, chars, ellipsis, flags)
			     + quotes);
		bfill = centre_pad_begin(r, width_max, fullwidth);
	}

	width = 0;
	trunc = 0;
	utf8lite_graphscan_make(&scan, text);

	while (!trunc && utf8lite_graphscan_advance(&scan)) {
		TRY(utf8lite_graph_measure(&scan.current, flags, &w));

		if (width > chars - w) {
			w = ellipsis;
			TRY(utf8lite_render_string(r, ellipsis_str));
			trunc = 1;
		} else {
			TRY(utf8lite_render_graph(r, &scan.current));
		}

		width += w;
	}

	if (!trim) {
		pad_spaces(r, width_max - width - quotes - bfill);
	}

	ans = mkCharLenCE((char *)r->string, r->length, CE_UTF8);
	utf8lite_render_clear(r);
exit:
	CHECK_ERROR(err);
	return ans;
}


SEXP rutf8_text_rformat(struct utf8lite_render *r,
			const struct utf8lite_text *text, int trim,
			int chars, int width_max, int quote,
			int utf8, int flags)
{
	SEXP ans = R_NilValue;
	struct utf8lite_graphscan scan;
	const char *ellipsis_str;
	int err = 0, w, width, quotes, ellipsis, trunc;

	quotes = quote ? 2 : 0;
	ellipsis = utf8 ? 1 : 3;
	ellipsis_str = utf8 ? RUTF8_ELLIPSIS : "...";

	utf8lite_graphscan_make(&scan, text);
	utf8lite_graphscan_skip(&scan);
	width = 0;
	trunc = 0;

	while (!trunc && utf8lite_graphscan_retreat(&scan)) {
		TRY(utf8lite_graph_measure(&scan.current, flags, &w));

		if (width > chars - w) {
			w = ellipsis;
			trunc = 1;
		}

		width += w;
	}

	if (!trim) {
		pad_spaces(r, width_max - width - quotes);
	}

	if (trunc) {
		TRY(utf8lite_render_string(r, ellipsis_str));
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
