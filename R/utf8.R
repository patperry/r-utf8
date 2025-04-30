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


# converts a character vector from its declared encoding to UTF-8


#' UTF-8 Character Encoding
#'
#' @description
#' UTF-8 text encoding and validation
#'
#' `as_utf8()` converts a character object from its declared encoding to a
#' valid UTF-8 character object, or throws an error if no conversion is
#' possible. If `normalize = TRUE`, then the text gets transformed to
#' Unicode composed normal form (NFC) after conversion to UTF-8.
#'
#' `utf8_valid()` tests whether the elements of a character object can be
#' translated to valid UTF-8 strings.
#'
#' @aliases as_utf8 utf8_valid
#' @param x character object.
#' @param normalize a logical value indicating whether to convert to Unicode
#'   composed normal form (NFC).
#' @return For `as_utf8()`, the result is a character object with the same
#'   attributes as `x` but with `Encoding` set to `"UTF-8"`.
#'
#'   For `utf8_valid()` a logical object with the same `names`,
#'   `dim`, and `dimnames` as `x`.
#' @seealso [utf8_normalize()], [iconv()].
#' @examples
#'
#' # the second element is encoded in latin-1, but declared as UTF-8
#' x <- c("fa\u00E7ile", "fa\xE7ile", "fa\xC3\xA7ile")
#' Encoding(x) <- c("UTF-8", "UTF-8", "bytes")
#'
#' # attempt to convert to UTF-8 (fails)
#' \dontrun{as_utf8(x)}
#'
#' y <- x
#' Encoding(y[2]) <- "latin1" # mark the correct encoding
#' as_utf8(y) # succeeds
#'
#' # test for valid UTF-8
#' utf8_valid(x)
#'
#' @export as_utf8
as_utf8 <- function(x, normalize = FALSE) {
  ans <- .Call(rutf8_as_utf8, x)
  if (normalize) {
    ans <- utf8_normalize(ans)
  }
  ans
}

# test whether the elements can be converted to valid UTF-8
#' @rdname as_utf8
#' @export
utf8_valid <- function(x) {
  .Call(rutf8_utf8_valid, x)
}

# gets the width; NA for invalid or nonprintable sequences


#' Measure the Character String Width
#'
#' Compute the display widths of the elements of a character object.
#'
#' `utf8_width()` returns the printed widths of the elements of a character
#' object on a UTF-8 device (or on an ASCII device when `output_utf8()` is
#' `FALSE`), when printed with `utf8_print()`. If the string is not
#' printable on the device, for example if it contains a control code like
#' `"\n"`, then the result is `NA`. If `encode = TRUE`, the
#' default, then the function returns the widths of the encoded elements via
#' `utf8_encode()`); otherwise, the function returns the widths of the
#' original elements.
#'
#' @param x character object.
#' @param encode whether to encode the object before measuring its width.
#' @param quote whether to quote the object before measuring its width.
#' @param utf8 logical scalar indicating whether to determine widths assuming a
#'   UTF-8 capable display (ASCII-only otherwise), or `NULL` to format for
#'   output capabilities as determined by `output_utf8()`.
#' @return An integer object, with the same `names`, `dim`, and
#'   `dimnames` as `x`.
#' @seealso [utf8_print()].
#' @examples
#'
#' # the second element is encoded in latin-1, but declared as UTF-8
#' x <- c("fa\u00E7ile", "fa\xE7ile", "fa\xC3\xA7ile")
#' Encoding(x) <- c("UTF-8", "UTF-8", "bytes")
#'
#' # get widths
#' utf8_width(x)
#' utf8_width(x, encode = FALSE)
#' utf8_width('"')
#' utf8_width('"', quote = TRUE)
#'
#' @export utf8_width
utf8_width <- function(x, encode = TRUE, quote = FALSE, utf8 = NULL) {
  with_rethrow({
    encode <- as_option("encode", encode)
    quote <- as_option("quote", quote)
    utf8 <- as_output_utf8("utf8", utf8)
  })
  .Call(rutf8_utf8_width, x, encode, quote, utf8)
}



#' Text Normalization
#'
#' Transform text to normalized form, optionally mapping to lowercase and
#' applying compatibility maps.
#'
#' `utf8_normalize()` converts the elements of a character object to Unicode
#' normalized composed form (NFC) while applying the character maps specified
#' by the `map_case`, `map_compat`, `map_quote`, and
#' `remove_ignorable` arguments.
#'
#' @param x character object.
#' @param map_case a logical value indicating whether to apply Unicode case
#'   mapping to the text. For most languages, this transformation changes
#'   uppercase characters to their lowercase equivalents.
#' @param map_compat a logical value indicating whether to apply Unicode
#'   compatibility mappings to the characters, those required for NFKC and NFKD
#'   normal forms.
#' @param map_quote a logical value indicating whether to replace curly single
#'   quotes and Unicode apostrophe characters with ASCII apostrophe (U+0027).
#' @param remove_ignorable a logical value indicating whether to remove Unicode
#'   "default ignorable" characters like zero-width spaces and soft hyphens.
#' @return The result is a character object with the same attributes as
#'   `x` but with `Encoding` set to `"UTF-8"`.
#' @seealso [as_utf8()].
#' @examples
#'
#' angstrom <- c("\u00c5", "\u0041\u030a", "\u212b")
#' utf8_normalize(angstrom) == "\u00c5"
#'
#' @export utf8_normalize
utf8_normalize <- function(x, map_case = FALSE, map_compat = FALSE,
                           map_quote = FALSE, remove_ignorable = FALSE) {
  with_rethrow({
    x <- as_utf8(x, normalize = FALSE)
    map_case <- as_option("map_case", map_case)
    map_compat <- as_option("map_compat", map_compat)
    map_quote <- as_option("map_quote", map_quote)
    remove_ignorable <- as_option("remove_ignorable", remove_ignorable)
  })

  .Call(
    rutf8_utf8_normalize, x, map_case, map_compat, map_quote,
    remove_ignorable
  )
}
