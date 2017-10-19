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
as_utf8 <- function(x, normalize = FALSE)
{
    ans <- .Call(C_utf8_coerce, x)
    if (normalize) {
        ans <- utf8_normalize(ans)
    }
    ans
}

# encode an R character string in a form suitable for display
# in the current locale (determined by LC_CTYPE)
utf8_encode <- function(x, display = FALSE)
{
    utf8 <- (Sys.getlocale("LC_CTYPE") != "C")
    .Call(C_utf8_encode, x, display, utf8)
}


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

    .Call(C_utf8_format, x, trim, chars, justify, width, na.encode,
          quote, na.print, utf8)
}


utf8_print <- function(x, chars = NULL, quote = TRUE, na.print = NULL,
                       print.gap = NULL, right = FALSE, max = NULL,
                       display = TRUE, ...)
{
    if (is.null(x)) {
        return(invisible(NULL))
    }

    if (!is.character(x)) {
        stop("argument is not a character object")
    }

    with_rethrow({
        chars <- as_chars("chars", chars)
        quote <- as_option("quote", quote)
        na.print <- as_na_print("na.print", na.print)
        print.gap <- as_print_gap("print_gap", print.gap)
        right <- as_option("right", right)
        max <- as_max_print("max", max)
        display <- as_option("display", display)
    })

    if (is.null(print.gap)) {
        print.gap <- 1L
    }
    if (is.null(max)) {
        max <- getOption("max.print")
    }
    width <- getOption("width")
    stdout <- as.integer(stdout()) == 1
    
    n <- length(x)
    dim <- dim(x)
    vec <- is.null(dim(x)) || length(dim(x)) == 1
    justify <- if (right) "right" else "left"

    fmt <- utf8_format(x, trim = !vec, chars = chars,
                       justify = justify, na.encode = TRUE,
                       quote = quote, na.print = na.print)
    names <- names(fmt)
    dimnames <- dimnames(fmt)

    if (is.null(dim) || length(dim) == 1) {
        if (length(dim) == 1 && !is.null(dimnames)) {
            names <- dimnames[[1]]
        }
    } else {
        if (is.null(dimnames)) {
            dimnames <- vector("list", length(dim))
        }

        for (i in seq_along(dim)) {
            d <- dim[[i]]
            ix <- seq_len(d)
            if (is.null(dimnames[[i]]) && d > 0) {
                if (i == 1) {
                    dimnames[[i]] <- format(paste0("[", ix, ",]"),
                                            justify = "right")
                } else if (i == 2) {
                    dimnames[[i]] <- paste0("[,", ix, "]")
                } else {
                    dimnames[[i]] <- as.character(ix)
                }
            }
        }
    }

    fmt <- utf8_encode(fmt, display)
    if (is.null(dim)) {
        names <- utf8_encode(names, display)
        names(fmt) <- names
    } else {
        dimnames <- lapply(dimnames, utf8_encode, display = display)
        dimnames(fmt) <- dimnames
    }

    nprint <- 0L

    if (vec) {
        # vector
        if (n == 0) {
            cat("character(0)\n")
            return(invisible(x))
        }

        if (is.null(names) && n > 0) {
            labels <- utf8_format(paste0("[", seq_len(n), "]"),
                                  justify = "right")
        } else {
            labels <- names
        }

        width <- getOption("width")
        namewidth <- max(0, utf8_width(labels))
        elt <- max(0, utf8_width(fmt))

        if (!is.null(names)) {
            ncol <- max(1, width %/% (max(namewidth, elt) + print.gap))
            extra <- n %% ncol

            off <- 0
            while (off + ncol <= n && nprint < max) {
                ix <- (off+1):(off+ncol)
                mat <- matrix(fmt[ix], ncol = ncol, byrow = TRUE,
                              dimnames = list(NULL, names[ix]))

                np <- .Call(C_print_table, mat, print.gap, right,
                            max - nprint, width, stdout)
                nprint <- nprint + np
                off <- off + ncol
            }

            if (extra > 0 && nprint < max) {
                ix <- n - extra + seq_len(extra)
                last <- rbind(as.vector(fmt[ix]))
                rownames(last) <- NULL
                colnames(last) <-  names[ix]

                np <- .Call(C_print_table, last, print.gap, right,
                            max - nprint, width, stdout)
                nprint <- nprint + np
            }
        } else {
            ncol <- max(1, (width - namewidth) %/% (elt + print.gap))
            extra <- n %% ncol

            mat <- matrix(fmt[seq_len(n - extra)], ncol = ncol, byrow = TRUE)
            rownames(mat) <- labels[seq(from = 1, by = ncol,
                                        length.out = nrow(mat))]
            np <- .Call(C_print_table, mat, print.gap, right, max, width,
                        stdout)
            nprint <- nprint + np

            if (extra > 0 && nprint < max) {
                last <- rbind(as.vector(fmt[n - extra  + seq_len(extra)]))
                rownames(last) <- labels[n - extra + 1]
                np <- .Call(C_print_table, last, print.gap, right,
                            max - nprint, width, stdout)
                nprint <- nprint + np
            }
        }

    } else if (length(dim) == 2) {
        # matrix
        if (all(dim == 0)) {
            cat("<0 x 0 matrix>\n")
        } else {
            nprint <- .Call(C_print_table, fmt, print.gap, right, max, width,
                            stdout)
        }
    } else {
        nrow <- dim[1]
        ncol <- dim[2]

        # array
        if (any(dim == 0)) {
            cat(sprintf("<%s array>\n", paste(dim, collapse = " x ")))
        } else {
            off <- 0

            base <- c(NA, NA, rep(1, length(dim) - 2))
            label <- vector("character", length(dim))
            for (r in 3:length(dim)) {
                label[[r]] <- dimnames[[r]][[1]]
            }

            while (off + nrow * ncol <= n && nprint < max) {
                cat(paste(label, collapse = ", "), "\n\n", sep = "")

                ix <- off + seq_len(nrow * ncol)
                mat <- matrix(fmt[ix], nrow, ncol, dimnames = dimnames[1:2])
                np <- .Call(C_print_table, mat, print.gap, right, max - nprint,
                            width, stdout)
                nprint <- nprint + np
                off <- off + (nrow * ncol)

                r <- 3L
                while (r < length(dim) && base[r] == dim[r]) {
                    base[r] <- 1L
                    label[r] <- dimnames[[r]][[1]]
                    r <- r + 1L
                }
                if (base[r] < dim[r]) {
                    base[r] <- base[r] + 1L
                    label[r] <- dimnames[[r]][[base[r]]]
                }
                cat("\n")
            }
        }
    }

    if (nprint < length(x)) {
        cat(sprintf(" [ reached getOption(\"max.print\") -- omitted %d entries ]\n",
                    length(x) - nprint))
    }

    invisible(x)
}


# test whether the elements can be converted to valid UTF-8
utf8_valid <- function(x)
{
    .Call(C_utf8_valid, x)
}

# gets the width; NA for invalid or nonprintable sequences
utf8_width <- function(x, encode = TRUE, quote = FALSE)
{
    with_rethrow({
        encode <- as_option("encode", encode)
        quote <- as_option("quote", quote)
    })
    
    if (encode) {
        x <- utf8_encode(x)
    }
    utf8 <- (Sys.getlocale("LC_CTYPE") != "C")
    .Call(C_utf8_width, x, quote, utf8)
}


utf8_normalize <- function(x, map_case = FALSE, map_compat = FALSE,
                           map_quote = FALSE, remove_ignorable = FALSE)
{
    with_rethrow({
        x <- as_utf8(x, normalize = FALSE)
        map_case <- as_option("map_case", map_case)
        map_compat <- as_option("map_compat", map_compat)
        map_quote <- as_option("map_quote", map_quote)
        remove_ignorable <- as_option("remove_ignorable", remove_ignorable)
    })

    .Call(C_utf8_normalize, x, map_case, map_compat, map_quote,
          remove_ignorable)
}
