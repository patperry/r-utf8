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

utf8_format <- function(x, trim = FALSE, chars = NULL, justify = "left",
                        width = NULL, na.encode = TRUE, quote = FALSE,
                        na.print = NULL, print.gap = NULL, ...)
{
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
    })

    utf8 <- Sys.getlocale("LC_CTYPE") != "C"

    if (is.null(chars) && length(x) > 0) {
        linewidth <- getOption("width")
        ellipsis <- if (utf8) 1 else 3
        quotes <- if (quote) 2 else 0
        gap <- if (is.null(print.gap)) 1 else NULL

        dim <- dim(x)
        dimnames <- dimnames(x)

        if (is.null(dim) || length(dim) == 1) {
            names <- if (is.null(dimnames)) names(x) else dimnames[[1]]
            if (is.null(names)) {
                namewidth <- floor(log10(length(x)) + 1) + 2
            } else {
                namewidth <- 0
            }
        } else {
            names <- if (is.null(dimnames)) NULL else dimnames[[1]]
            if (is.null(names)) {
                namewidth <- floor(log10(length(x)) + 1) + 3 # add comma
            } else {
                namewidth <- max(0, utf8_width(names))
            }
        }

        chars <- (linewidth - ellipsis - quotes - gap - namewidth)
        chars <- max(chars, 12)
    }

    .Call(rutf8_utf8_format, x, trim, chars, justify, width, na.encode,
          quote, na.print, utf8)
}