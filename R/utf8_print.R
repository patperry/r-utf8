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
                       names = NULL, rownames = NULL, escapes = NULL,
                       display = TRUE, style = TRUE, utf8 = NULL, ...)
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
        names <- as_style("names", names)
        rownames <- as_style("rownames", rownames)
        escapes <- as_style("escapes", escapes)
        display <- as_option("display", display)
        style <- as_option("style", style)
        utf8 <- as_output_utf8("utf8", utf8)
    })

    if (is.null(print.gap)) {
        print.gap <- 1L
    }
    if (is.null(max)) {
        max <- getOption("max.print")
    }

    if (!output_ansi()) {
        style <- FALSE
    }

    if (!output_utf8()) {
        utf8 <- FALSE
    }

    # truncate character objects
    justify <- if (right) "right" else "left"
    fmt <- utf8_format(x, trim = TRUE, chars = chars,
                       justify = justify, na.encode = FALSE,
                       quote = quote, utf8 = utf8)

    dim <- dim(x)
    if (is.null(dim) || length(dim) == 1) {
        nprint <- print_vector(fmt, quote = quote, na.print = na.print,
                               print.gap = print.gap, right = right,
                               max = max, names = names, rownames = rownames,
                               escapes = escapes, display = display,
                               style = style, utf8 = utf8)
    } else if (length(dim) == 2) {
        nprint <- print_matrix(fmt, quote = quote, na.print = na.print,
                               print.gap = print.gap, right = right,
                               max = max, names = names, rownames = rownames,
                               escapes = escapes, display = display,
                               style = style, utf8 = utf8)
    } else {
        nprint <- print_array(fmt, quote = quote, na.print = na.print,
                              print.gap = print.gap, right = right,
                              max = max, names = names, rownames = rownames,
                              escapes = escapes, display = display,
                              style = style, utf8 = utf8)
    }

    n <- length(x)
    if (nprint < n) {
        cat(sprintf(
            " [ reached getOption(\"max.print\") -- omitted %d entries ]\n",
            n - nprint))
    }

    invisible(x)
}


print_vector <- function(x, quote, na.print, print.gap, right, max,
                         names, rownames, escapes, display, style, utf8)
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
        nprint <- print_vector_named(x, quote = quote, na.print = na.print,
                                     print.gap = print.gap, right = right,
                                     max = max, names = names,
                                     rownames = rownames, escapes = escapes,
                                     display = display, style = style,
                                     utf8 = utf8)
    } else {
        nprint <- print_vector_unnamed(x, quote = quote, na.print = na.print,
                                       print.gap = print.gap, right = right,
                                       max = max, names = names,
                                       rownames = rownames, escapes = escapes,
                                       display = display, style = style,
                                       utf8 = utf8)
    }

    nprint
}


print_vector_named <- function(x, quote, na.print, print.gap, right, max,
                               names, rownames, escapes, display, style, utf8)
{
    n <- length(x)
    nm <- names(x)
    namewidth <- max(0L, utf8_width(nm, utf8 = utf8))
    eltwidth <- element_width(x, quote = quote, na.print = na.print,
                              utf8 = utf8)
    width <- max(eltwidth, namewidth)

    linewidth <- getOption("width")
    ncol <- max(1L, linewidth %/% (width + print.gap))
    extra <- n %% ncol

    nprint <- 0L
    off <- 0L
    while (off + ncol <= n && nprint < max) {
        ix <- (off + 1L):(off + ncol)
        mat <- matrix(x[ix], ncol = ncol, byrow = TRUE,
                      dimnames = list(NULL, nm[ix]))
        np <- print_table(mat, width = width, quote = quote,
                          na.print = na.print, print.gap = print.gap,
                          right = right, max = max - nprint,
                          names = names, rownames = rownames,
                          escapes = escapes, display = display,
                          style = style, utf8 = utf8)
        nprint <- nprint + np
        off <- off + ncol
    }

    if (extra > 0L && nprint < max) {
        ix <- n - extra + seq_len(extra)
        last <- rbind(as.vector(x[ix]))
        rownames(last) <- NULL
        colnames(last) <-  nm[ix]

        np <- print_table(last, width = width, quote = quote,
                          na.print = na.print, print.gap = print.gap,
                          right = right, max = max - nprint,
                          names = names, rownames = rownames,
                          escapes = escapes, display = display,
                          style = style, utf8 = utf8)
        nprint <- nprint + np
    }

    nprint
}


print_vector_unnamed <- function(x, quote, na.print, print.gap, right,
                                 max, names, rownames, escapes = escapes,
                                 display, style, utf8)
{
    n <- length(x)
    nm <- utf8_format(paste0("[", seq_len(n), "]"), justify = "right",
                      utf8 = utf8)
    namewidth <- max(0L, utf8_width(nm, utf8 = utf8))
    width <- element_width(x, quote = quote, na.print = na.print, utf8 = utf8)

    linewidth <- getOption("width")
    ncol <- max(1L, (linewidth - namewidth) %/% (width + print.gap))
    extra <- n %% ncol

    mat <- matrix(x[seq_len(n - extra)], ncol = ncol, byrow = TRUE)
    rownames(mat) <- nm[seq(from = 1L, by = ncol, length.out = nrow(mat))]

    nprint <- print_table(mat, width = width, quote = quote,
                          na.print = na.print, print.gap = print.gap,
                          right = right, max = max, names = names,
                          rownames = rownames, escapes = escapes,
                          display = display, style = style, utf8 = utf8)

    if (extra > 0L && nprint < max) {
        last <- rbind(as.vector(x[n - extra  + seq_len(extra)]))
        rownames(last) <- nm[n - extra + 1]
        np <- print_table(last, width = width, quote = quote,
                          na.print = na.print, print.gap = print.gap,
                          right = right, max = max - nprint, names = names,
                          rownames = rownames, escapes = escapes,
                          display = display, style = style, utf8 = utf8)
        nprint <- nprint + np
    }

    nprint
}


element_width <- function(x, quote, na.print, utf8)
{
    width <- max(0L, utf8_width(x, encode = TRUE, quote = quote, utf8 = utf8),
                 na.rm = TRUE)
    if (anyNA(x)) {
        if (is.null(na.print)) {
            na.print <- if (quote) "NA" else "<NA>"
        }
        width <- max(width, utf8_width(na.print, utf8 = utf8))
    }
    width
}


print_matrix <- function(x, quote, na.print, print.gap, right, max,
                         names, rownames, escapes, display, style, utf8)
{
    if (all(dim(x) == 0)) {
        cat("<0 x 0 matrix>\n")
        return(0L)
    }
    x <- set_dimnames(x)
    print_table(x, width = 0L, quote = quote, na.print = na.print,
                print.gap = print.gap, right = right, max = max,
                names = names, rownames = rownames, escapes = escapes,
                display = display, style = style, utf8 = utf8)
}


print_array <- function(x, quote, na.print, print.gap, right, max,
                        names, rownames, escapes, display, style, utf8)
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

    base <- c(NA, NA, rep(1, length(dim) - 2))
    label <- vector("character", length(dim))
    for (r in 3:length(dim)) {
        label[[r]] <- dimnames[[r]][[1]]
    }

    nprint <- 0L
    off <- 0L
    while (off + nrow * ncol <= n && nprint < max) {
        cat(paste(label, collapse = ", "), "\n\n", sep = "")

        ix <- off + seq_len(nrow * ncol)
        mat <- matrix(x[ix], nrow, ncol, dimnames = dimnames[1:2])
        np <- print_table(mat, width = 0L, quote = quote, na.print = na.print,
                          print.gap = print.gap, right = right,
                          max = max - nprint, names = names,
                          rownames = rownames, escapes = escapes,
                          display = display, style = style, utf8 = utf8)
        nprint <- nprint + np
        off <- off + (nrow * ncol)

        r <- 3L
        while (r < length(dim) && base[r] == dim[r]) {
            base[r] <- 1L
            label[r] <- dimnames[[r]][[1L]]
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


print_table <- function(x, width, quote, na.print, print.gap, right, max,
                        names, rownames, escapes, display, style, utf8)
{
    width <- as.integer(width)
    if (is.null(na.print)) {
        na.print <- if (quote) "NA" else "<NA>"
        na.name.print <- "<NA>"
    } else {
        na.name.print <- na.print
    }
    print.gap <- as.integer(print.gap)
    max <- as.integer(max)

    if (!is.null(rownames(x))) {
        rownames(x)[is.na(rownames(x))] <- na.name.print
    }
    if (!is.null(colnames(x))) {
        colnames(x)[is.na(colnames(x))] <- na.name.print
    }

    linewidth <- getOption("width")
    str <- .Call(rutf8_render_table, x, width, quote, na.print, print.gap,
                 right, max, names, rownames, escapes, display, style,
                 utf8, linewidth)
    cat(str)

    nprint <- min(max, length(x))
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
