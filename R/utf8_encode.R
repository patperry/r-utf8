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



#' Encode Character Object as for UTF-8 Printing
#'
#' Escape the strings in a character object, optionally adding quotes or
#' spaces, adjusting the width for display.
#'
#' `utf8_encode()` encodes a character object for printing on a UTF-8 device
#' by escaping controls characters and other non-printable characters. When
#' `display = TRUE`, the function optimizes the encoding for display by
#' removing default ignorable characters (soft hyphens, zero-width spaces,
#' etc.) and placing zero-width spaces after wide emoji. When
#' `output_utf8()` is `FALSE` the function escapes all non-ASCII
#' characters and gives the same results on all platforms.
#'
#' @param x character object.
#' @param width integer giving the minimum field width; specify `NULL` or
#'   `NA` for no minimum.
#' @param quote logical scalar indicating whether to surround results with
#'   double-quotes and escape internal double-quotes.
#' @param justify justification; one of `"left"`, `"right"`,
#'   `"centre"`, or `"none"`. Can be abbreviated.
#' @param escapes a character string specifying the display style for the
#'   backslash escapes, as an ANSI SGR parameter string, or NULL for no styling.
#' @param display logical scalar indicating whether to optimize the encoding
#'   for display, not byte-for-byte data transmission.
#' @param utf8 logical scalar indicating whether to encode for a UTF-8 capable
#'   display (ASCII-only otherwise), or `NULL` to encode for output
#'   capabilities as determined by `output_utf8()`.
#' @return A character object with the same attributes as `x` but with
#'   `Encoding` set to `"UTF-8"`.
#' @seealso [utf8_print()].
#' @examples
#'
#' # the second element is encoded in latin-1, but declared as UTF-8
#' x <- c("fa\u00E7ile", "fa\xE7ile", "fa\xC3\xA7ile")
#' Encoding(x) <- c("UTF-8", "UTF-8", "bytes")
#'
#' # encoding
#' utf8_encode(x)
#'
#' # add style to the escapes
#' cat(utf8_encode("hello\nstyled\\world", escapes = "1"), "\n")
#'
#' @export utf8_encode
utf8_encode <- function(x, width = 0L, quote = FALSE, justify = "left",
                        escapes = NULL, display = FALSE, utf8 = NULL) {
  if (is.null(x)) {
    return(NULL)
  }

  if (!is.character(x)) {
    stop("argument is not a character object")
  }

  with_rethrow({
    width <- as_integer_scalar("width", width)
    quote <- as_option("quote", quote)
    justify <- as_justify("justify", justify)
    escapes <- as_style("escapes", escapes)
    display <- as_option("display", display)
    utf8 <- as_output_utf8("utf8", utf8)
  })

  .Call(rutf8_utf8_encode, x, width, quote, justify, escapes, display, utf8)
}
