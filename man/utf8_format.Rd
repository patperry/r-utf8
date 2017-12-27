\name{utf8_format}
\title{UTF-8 Text Formatting}
\alias{utf8_format}
\description{
Format a character object for UTF-8 printing.
}
\usage{
utf8_format(x, trim = FALSE, chars = NULL, justify = "left",
            width = NULL, na.encode = TRUE, quote = FALSE,
            na.print = NULL, print.gap = NULL, utf8 = NULL, ...)
}
\arguments{
\item{x}{character object.}

\item{trim}{logical scalar indicating whether to suppress padding
    spaces around elements.}

\item{chars}{integer scalar indicating the maximum number of
    character units to display.  Wide characters like emoji take
    two character units; combining marks and default ignorables
    take none. Longer strings get truncated and suffixed or prefixed
    with an ellipsis (\code{"..."} or \code{"\u2026"}, whichever is most
    appropriate for the current character locale). Set to \code{NULL} to
    limit output to the line width as determined by
    \code{getOption("width")}.}

\item{justify}{justification; one of \code{"left"}, \code{"right"},
    \code{"centre"}, or \code{"none"}. Can be abbreviated.}

\item{width}{the minimum field width; set to \code{NULL} or
    \code{0} for no restriction.}

\item{na.encode}{logical scalar indicating whether to encode
    \code{NA} values as character strings.}

\item{quote}{logical scalar indicating whether to format for a context
    with surrounding double-quotes (\code{'"'}) and escaped internal
    double-quotes.}

\item{na.print}{character string (or \code{NULL}) indicating the
    encoding for \code{NA} values. Ignored when \code{na.encode}
    is \code{FALSE}.}

\item{print.gap}{non-negative integer (or \code{NULL}) giving the
    number of spaces in gaps between columns; set to \code{NULL}
    or \code{1} for a single space.}

\item{utf8}{logical scalar indicating whether to format for a UTF-8
    capable display (ASCII-only otherwise), or \code{NULL} to
    format for output capabilities as determined by \code{output_utf8()}.}

\item{...}{further arguments passed from other methods. Ignored.}
}
\details{
\code{utf8_format} formats a character object for printing, optionally
truncating long character strings.
}
\value{
A character object with the same attributes as \code{x} but with
\code{Encoding} set to \code{"UTF-8"} for elements that can be
converted to valid UTF-8 and \code{"bytes"} for others.
}
\seealso{
\code{\link{utf8_print}}, \code{\link{utf8_encode}}.
}
\examples{
# the second element is encoded in latin-1, but declared as UTF-8
x <- c("fa\u00E7ile", "fa\xE7ile", "fa\xC3\xA7ile")
Encoding(x) <- c("UTF-8", "UTF-8", "bytes")

# formatting
utf8_format(x, chars = 3)
utf8_format(x, chars = 3, justify = "centre", width = 10)
utf8_format(x, chars = 3, justify = "right")
}
