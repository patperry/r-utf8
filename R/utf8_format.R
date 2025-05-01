#  Copyright 2017 Patrick O. Perry.
#
#  Licensed under the Apache License, Version 2.0 (the "License");
#  you may not use this file except in compliance with the License.
#  You may obtain a copy of the License at
#
#      http://www.apache.org/licenses/LICENSE-2.0
#
#  Unless required by applicable law or agreed to in writing, software
#  distributed under the License is distributed on an "AS IS" BASIS,
#  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
#  See the License for the specific language governing permissions and
#  limitations under the License.



#' UTF-8 Text Formatting
#'
#' Format a character object for UTF-8 printing.
#'
#' `utf8_format()` formats a character object for printing, optionally
#' truncating long character strings.
#'
#' @inheritParams rlang::args_dots_empty
#' @param x character object.
#' @param trim logical scalar indicating whether to suppress padding spaces
#'   around elements.
#' @param chars integer scalar indicating the maximum number of character units
#'   to display.  Wide characters like emoji take two character units; combining
#'   marks and default ignorables take none. Longer strings get truncated and
#'   suffixed or prefixed with an ellipsis (`"..."` or `"\u2026"`,
#'   whichever is most appropriate for the current character locale). Set to
#'   `NULL` to limit output to the line width as determined by
#'   `getOption("width")`.
#' @param justify justification; one of `"left"`, `"right"`,
#'   `"centre"`, or `"none"`. Can be abbreviated.
#' @param width the minimum field width; set to `NULL` or `0` for no
#'   restriction.
#' @param na.encode logical scalar indicating whether to encode `NA`
#'   values as character strings.
#' @param quote logical scalar indicating whether to format for a context with
#'   surrounding double-quotes (`'"'`) and escaped internal double-quotes.
#' @param na.print character string (or `NULL`) indicating the encoding
#'   for `NA` values. Ignored when `na.encode` is `FALSE`.
#' @param print.gap non-negative integer (or `NULL`) giving the number of
#'   spaces in gaps between columns; set to `NULL` or `1` for a single
#'   space.
#' @param utf8 logical scalar indicating whether to format for a UTF-8 capable
#'   display (ASCII-only otherwise), or `NULL` to format for output
#'   capabilities as determined by `output_utf8()`.
#' @return A character object with the same attributes as `x` but with
#'   `Encoding` set to `"UTF-8"` for elements that can be converted to
#'   valid UTF-8 and `"bytes"` for others.
#' @seealso [utf8_print()], [utf8_encode()].
#' @examples
#'
#' # the second element is encoded in latin-1, but declared as UTF-8
#' x <- c("fa\u00E7ile", "fa\xE7ile", "fa\xC3\xA7ile")
#' Encoding(x) <- c("UTF-8", "UTF-8", "bytes")
#'
#' # formatting
#' utf8_format(x, chars = 3)
#' utf8_format(x, chars = 3, justify = "centre", width = 10)
#' utf8_format(x, chars = 3, justify = "right")
#'
#' @export utf8_format
utf8_format <- function(
  x,
  ...,
  trim = FALSE,
  chars = NULL,
  justify = "left",
  width = NULL,
  na.encode = TRUE,
  quote = FALSE,
  na.print = NULL,
  print.gap = NULL,
  utf8 = NULL
) {
  stopifnot(...length() == 0)

  if (is.null(x)) {
    return(NULL)
  }

  if (!is.character(x)) {
    stop("argument is not a character object")
  }

  with_rethrow({
    trim <- as_option("trim", trim)
    chars <- as_chars("chars", chars)
    justify <- as_justify("justify", justify)
    width <- as_integer_scalar("width", width)
    na.encode <- as_option("na.encode", na.encode)
    quote <- as_option("quote", quote)
    na.print <- as_na_print("na.print", na.print)
    print.gap <- as_print_gap("print_gap", print.gap)
    utf8 <- as_output_utf8("utf8", utf8)
  })

  ellipsis <- "\u2026"
  iellipsis <- iconv(ellipsis, "UTF-8", "")
  if (is.na(iellipsis) || identical(iellipsis, "...")) {
    ellipsis <- "..."
    wellipsis <- 3L
  } else {
    wellipsis <- 1L
  }

  if (is.null(chars) && length(x) > 0) {
    linewidth <- getOption("width")
    quotes <- if (quote) 2 else 0
    gap <- if (is.null(print.gap)) 1 else NULL

    dim <- dim(x)
    dimnames <- dimnames(x)
    names <- if (is.null(dimnames)) names(x) else dimnames[[1]]

    if (is.null(names)) {
      comma <- length(dim) > 1
      namewidth <- floor(log10(length(x)) + 1) + 2 + comma
    } else if (length(dim) > 1) {
      namewidth <- max(0, utf8_width(names, utf8 = utf8))
    } else {
      namewidth <- 0
    }

    chars <- (linewidth - wellipsis - quotes - gap - namewidth)
    chars <- max(chars, 12)
  }

  .Call(
    rutf8_utf8_format, x, trim, chars, justify, width, na.encode,
    quote, na.print, ellipsis, wellipsis, utf8
  )
}
