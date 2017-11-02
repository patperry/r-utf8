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
    
    # truncate character objects, replace NA by na.print
    justify <- if (right) "right" else "left"
    fmt <- utf8_format(x, trim = TRUE, chars = chars,
                       justify = justify, na.encode = TRUE,
                       quote = quote, na.print = na.print)

    dim <- dim(x)
    if (is.null(dim) || length(dim) == 1) {
        nprint <- print_vector(fmt, quote = quote, print.gap = print.gap,
                               right = right, max = max, display = display,
                               width = width, stdout = stdout)
    } else if (length(dim) == 2) {
        nprint <- print_matrix(fmt, quote = quote, print.gap = print.gap,
                               right = right, max = max, display = display)
    } else {
        nprint <- print_array(fmt, quote = quote, print.gap = print.gap,
                              right = right, max = max, display = display)
    }

    n <- length(x)
    if (nprint < n) {
        cat(sprintf(
            " [ reached getOption(\"max.print\") -- omitted %d entries ]\n",
            n - nprint))
    }

    invisible(x)
}


print_vector <- function(x, quote = TRUE, print.gap = NULL, right = FALSE,
                         max = NULL, display = TRUE, width = NULL,
                         stdout = TRUE)
{
    if (length(x) == 0) {
        cat("character(0)\n")
        return(0L)
    }

    # drop dim, convert dimnames to names
    if (!is.null(dim(x))) {
        x <- c(x)
    }

    if (!is.null(names(x))) {
        nprint <- print_vector_named(x, quote = quote, print.gap = print.gap,
                                     right = right, max = max,
                                     display = display, width = width,
                                     stdout = stdout)
    } else {
        nprint <- print_vector_unnamed(x, quote = quote, print.gap = print.gap,
                                       right = right, max = max,
                                       display = display, width = width,
                                       stdout = stdout)
    }

    nprint
}


print_vector_named <- function(x, quote = TRUE, print.gap = NULL,
                               right = FALSE, max = NULL, display = TRUE,
                               width = NULL, stdout = TRUE)
{
    justify <- if (right) "right" else "left"
    fmt <- utf8_encode(x, width = NULL, quote = quote, justify = justify,
                       display = display)

    names <- utf8_encode(names(fmt), display = display)
    namewidth <- max(0, utf8_width(names))

    elt <- max(0, utf8_width(fmt))
    ncol <- max(1, width %/% (max(namewidth, elt) + print.gap))
    n <- length(x)
    extra <- n %% ncol

    nprint <- 0L
    off <- 0
    while (off + ncol <= n && nprint < max) {
        ix <- (off+1):(off+ncol)
        mat <- matrix(fmt[ix], ncol = ncol, byrow = TRUE,
                      dimnames = list(NULL, names[ix]))

        np <- .Call(rutf8_print_table, mat, print.gap, right,
                    max - nprint, width, stdout)
        nprint <- nprint + np
        off <- off + ncol
    }

    if (extra > 0 && nprint < max) {
        ix <- n - extra + seq_len(extra)
        last <- rbind(as.vector(fmt[ix]))
        rownames(last) <- NULL
        colnames(last) <-  names[ix]

        np <- .Call(rutf8_print_table, last, print.gap, right,
                    max - nprint, width, stdout)
        nprint <- nprint + np
    }

    nprint
}


print_vector_unnamed <- function(x, quote = TRUE, print.gap = NULL,
                                 right = FALSE, max = NULL, display = TRUE,
                                 width = NULL, stdout = TRUE)
{
    fmt <- utf8_encode(x, width = NULL, quote = quote)
    elt <- max(0, utf8_width(fmt, encode = FALSE))

    n <- length(x)
    names <- utf8_format(paste0("[", seq_len(n), "]"), justify = "right")
    namewidth <- max(0, utf8_width(names, encode = FALSE))

    ncol <- max(1, (width - namewidth) %/% (elt + print.gap))
    extra <- n %% ncol

    mat <- matrix(x[seq_len(n - extra)], ncol = ncol, byrow = TRUE)
    rownames(mat) <- names[seq(from = 1, by = ncol, length.out = nrow(mat))]

    nprint <- print_table(mat, width = elt, quote = quote,
                          print.gap = print.gap, right = right, max = max,
                          display = display)

    if (extra > 0 && nprint < max) {
        last <- rbind(as.vector(x[n - extra  + seq_len(extra)]))
        rownames(last) <- names[n - extra + 1]
        np <- print_table(last, width = elt, quote = quote,
                          print.gap = print.gap, right = right,
                          max = max - nprint, display = display)
        nprint <- nprint + np
    }

    nprint
}


print_matrix <- function(x, quote, print.gap, right, max, display)
{
    if (all(dim(x) == 0)) {
        cat("<0 x 0 matrix>\n")
        return(0L)
    }
    x <- set_dimnames(x)
    print_table(x, width = 0L, quote = quote, print.gap = print.gap,
                right = right, max = max, display = display)
}


print_array <- function(x, quote, print.gap, right, max, display)
{
    n <- length(x)
    dim <- dim(x)
    if (any(dim == 0)) {
        cat(sprintf("<%s array>\n", paste(dim, collapse = " x ")))
        return(0L)
    }

    x <- set_dimnames(x)
    dimnames <- dimnames(x)

    nrow <- dim[1]
    ncol <- dim[2]
    off <- 0

    base <- c(NA, NA, rep(1, length(dim) - 2))
    label <- vector("character", length(dim))
    for (r in 3:length(dim)) {
        label[[r]] <- dimnames[[r]][[1]]
    }

    nprint <- 0L
    while (off + nrow * ncol <= n && nprint < max) {
        cat(paste(label, collapse = ", "), "\n\n", sep = "")

        ix <- off + seq_len(nrow * ncol)
        mat <- matrix(x[ix], nrow, ncol, dimnames = dimnames[1:2])
        np <- print_table(mat, width = 0L, quote = quote,
                          print.gap = print.gap, right = right,
                          max = max - nprint, display = display)
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

    nprint
}


print_table <- function(x, width, quote, print.gap, right, max, display)
{
    linewidth <- getOption("width")
    stdout <- as.integer(stdout()) == 1

    justify <- if (right) "right" else "left"
    fmt <- utf8_encode(x, width = width, quote = quote, justify = justify,
                       display = display)
    dimnames(fmt) <- lapply(dimnames(fmt), utf8_encode, display = display)

    nprint <- .Call(rutf8_print_table, fmt, print.gap, right, max,
                    linewidth, stdout)
    nprint

}


set_dimnames <- function(x)
{
    dim <- dim(x)
    dimnames <- dimnames(x)

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

    dimnames(x) <- dimnames
    
    x
}
