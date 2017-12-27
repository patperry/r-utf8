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
                        na.print = NULL, print.gap = NULL, utf8 = NULL, ...)
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
        utf8 <- as_output_utf8("utf8", utf8)
    })

    ellipsis <- "\u2026"
    if (is.na(iconv(ellipsis, "UTF-8", ""))) {
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

    .Call(rutf8_utf8_format, x, trim, chars, justify, width, na.encode,
          quote, na.print, ellipsis, wellipsis, utf8)
}
