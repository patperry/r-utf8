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


#' @rdname output_utf8
#' @export
output_ansi <- function() {
  # tty?
  if (isatty(stdout())) {
    # and not Windows GUI?
    return(.Platform$GUI != "Rgui")
  }

  # RStudio 1.1 or later with color enabled?
  if (!is.na(as.numeric(Sys.getenv("RSTUDIO_CONSOLE_COLOR")))) {
    # and output is stdout?
    return(stdout() == 1)
  }

  FALSE
}




#' Output Capabilities
#'
#' Test whether the output connection has ANSI style escape support or UTF-8
#' support.
#'
#' `output_ansi()` tests whether the output connection supports ANSI style
#' escapes. This is `TRUE` if the connection is a terminal and not the
#' Windows GUI. Otherwise, it is true if running in RStudio 1.1 or later with
#' ANSI escapes enabled, provided `stdout()` has not been redirected to
#' another connection by `sink()`.
#'
#' `output_utf8()` tests whether the output connection supports UTF-8. For
#' most platforms `l10n_info()$"UTF-8"` gives this information, but this
#' does not give an accurate result for Windows GUIs. To work around this, we
#' proceed as follows: \itemize{
#'
#' \item if the character locale
#' (`LC_CTYPE`) is `"C"`, then the result is `FALSE`;
#'
#' \item otherwise, if `l10n_info()$"UTF-8"` is `TRUE`, then the
#' result is `TRUE`;
#'
#' \item if running on Windows, then the result is `TRUE`;
#'
#' \item in all other cases the result is `FALSE`. }
#'
#' Strictly speaking,
#' UTF-8 support is always available on Windows GUI, but only a subset of UTF-8
#' is available (defined by the current character locale) when the output is
#' redirected by `knitr` or another process. Unfortunately, it is
#' impossible to set the character locale to UTF-8 on Windows. Further, the
#' `utf8` package only handles two character locales: C and UTF-8.  To get
#' around this, on Windows, we treat all non-C locales on that platform as
#' UTF-8. This liberal approach means that characters in the user's locale
#' never get escaped; others will get output as `<U+XXXX>`, with incorrect
#' values for `utf8_width()`.
#'
#' @aliases output_ansi output_utf8
#' @return A logical scalar indicating whether the output connection supports
#'   the given capability.
#' @seealso [.Platform()], [isatty()],
#' [l10n_info()], [Sys.getlocale()]
#' @examples
#'
#' # test whether ANSI style escapes or UTF-8 output are supported
#' cat("ANSI:", output_ansi(), "\n")
#' cat("UTF8:", output_utf8(), "\n")
#'
#' # switch to C locale
#' Sys.setlocale("LC_CTYPE", "C")
#' cat("ANSI:", output_ansi(), "\n")
#' cat("UTF8:", output_utf8(), "\n")
#'
#' # switch to native locale
#' Sys.setlocale("LC_CTYPE", "")
#'
#' tmp <- tempfile()
#' sink(tmp) # redirect output to a file
#' cat("ANSI:", output_ansi(), "\n")
#' cat("UTF8:", output_utf8(), "\n")
#' sink() # restore stdout
#'
#' # inspect the output
#' readLines(tmp)
#'
#' @export output_utf8
output_utf8 <- function() {
  # ASCII-only character locale?
  if (Sys.getlocale("LC_CTYPE") == "C") {
    return(FALSE)
  }

  # UTF-8 locale?
  if (l10n_info()$`UTF-8`) {
    return(TRUE)
  }

  # Windows?
  if (.Platform$OS.type == "windows") {
    # This isn't really the case, but there's no way to set the
    # locale to UTF-8 on Windows. In RGui and RStudio, UTF-8 is
    # always supported on stdout(); output through connections
    # gets translated through the native locale.
    return(TRUE)
  }

  FALSE
}
