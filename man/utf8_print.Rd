\name{utf8_print}
\title{Print UTF-8 Text}
\alias{utf8_print}
\description{
    Print a UTF-8 character object.
}
\usage{
utf8_print(x, chars = NULL, quote = TRUE, na.print = NULL,
           print.gap = NULL, right = FALSE, max = NULL,
           names = NULL, rownames = NULL, escapes = NULL,
           display = TRUE, style = TRUE, utf8 = NULL, ...)
}
\arguments{
    \item{x}{character object.}

    \item{chars}{integer scalar indicating the maximum number of
        character units to display.  Wide characters like emoji take
        two character units; combining marks and default ignorables
        take none. Longer strings get truncated and suffixed or prefixed
        with an ellipsis (\code{"..."} in C locale, \code{"\u2026"} in
        others). Set to \code{NULL} to limit output to the line width
        as determined by \code{getOption("width")}.}

    \item{quote}{logical scalar indicating whether to put surrounding
        double-quotes (\code{'"'}) around character strings and escape
        internal double-quotes.}

    \item{na.print}{character string (or \code{NULL}) indicating
        the encoding for \code{NA} values. Ignored when
        \code{na.encode} is \code{FALSE}.}

    \item{print.gap}{non-negative integer (or \code{NULL}) giving the
        number of spaces in gaps between columns; set to \code{NULL}
        or \code{1} for a single space.}

    \item{right}{logical scalar indicating whether to right-justify
        character strings.}

    \item{max}{non-negative integer (or \code{NULL}) indicating the
        maximum number of elements to print; set to
        \code{getOption("max.print")} if argument is \code{NULL}.}

    \item{names}{a character string specifying the display style for
        the (column) names, as an ANSI SGR parameter string.}

    \item{rownames}{a character string specifying the display style for
        the row names, as an ANSI SGR parameter string.}

    \item{escapes}{a character string specifying the display style for
        the backslash escapes, as an ANSI SGR parameter string.}

    \item{display}{logical scalar indicating whether to optimize the
        encoding for display, not byte-for-byte data transmission.}

    \item{style}{logical scalar indicating whether to apply ANSI
        terminal escape codes to style the output.  Ignored when
        \code{output_ansi()} is \code{FALSE}.}

    \item{utf8}{logical scalar indicating whether to optimize results
        for a UTF-8 capable display, or \code{NULL} to set as the
        result of \code{output_utf8()}. Ignored when \code{output_utf8()}
        is \code{FALSE}.}

    \item{...}{further arguments passed from other methods. Ignored.}
}
\details{
    \code{utf8_print} prints a character object after formatting it with
    \code{utf8_format}.

    For ANSI terminal output (when \code{output_ansi()} is \code{TRUE}),
    you can style the row and column names with the \code{rownames} and
    \code{names} parameters, specifying an ANSI SGR parameter string; see
    \url{https://en.wikipedia.org/wiki/ANSI_escape_code#SGR_(Select_Graphic_Rendition)_parameters}.
}
\value{
    The function returns \code{x} invisibly.
}
\seealso{
    \code{\link{utf8_format}}.
}
\examples{
# printing (assumes that output is capable of displaying Unicode 10.0.0)
print(intToUtf8(0x1F600 + 0:79)) # with default R print function
utf8_print(intToUtf8(0x1F600 + 0:79)) # with utf8_print, truncates line
utf8_print(intToUtf8(0x1F600 + 0:79), chars = 1000) # higher character limit

# in C locale, output ASCII (same results on all platforms)
oldlocale <- Sys.getlocale("LC_CTYPE")
invisible(Sys.setlocale("LC_CTYPE", "C")) # switch to C locale
utf8_print(intToUtf8(0x1F600 + 0:79))
invisible(Sys.setlocale("LC_CTYPE", oldlocale)) # switch back to old locale

# Mac and Linux only: style the names
# see https://en.wikipedia.org/wiki/ANSI_escape_code#SGR_(Select_Graphic_Rendition)_parameters
utf8_print(matrix(as.character(1:20), 4, 5),
           names = "1;4", rownames = "2;3")
}
