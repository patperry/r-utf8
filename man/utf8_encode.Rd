\name{utf8_encode}
\title{Encode Character Object as for UTF-8 Printing}
\alias{utf8_encode}
\description{
    Escape the strings in a character object, optionally adding
    quotes or spaces, adjusting the width for display.
}
\usage{
utf8_encode(x, width = 0L, quote = FALSE, justify = "left", escapes = NULL,
            display = FALSE, utf8 = NULL)
}
\arguments{
    \item{x}{character object.}

    \item{width}{integer giving the minimum field width; specify
        \code{NULL} or \code{NA} for no minimum.}

    \item{quote}{logical scalar indicating whether to surround results
        with double-quotes and escape internal double-quotes.}

    \item{justify}{justification; one of \code{"left"}, \code{"right"},
        \code{"centre"}, or \code{"none"}. Can be abbreviated.}

    \item{escapes}{a character string specifying the display style for
        the backslash escapes, as an ANSI SGR parameter string,
        or NULL for no styling.}

    \item{display}{logical scalar indicating whether to optimize the
        encoding for display, not byte-for-byte data transmission.}

    \item{utf8}{logical scalar indicating whether to encode for a UTF-8
        capable display (ASCII-only otherwise), or \code{NULL} to
        encode for output capabilities as determined by
        \code{output_utf8()}.}
}
\details{
    \code{utf8_encode} encodes a character object for printing on a UTF-8
    device by escaping controls characters and other non-printable
    characters. When \code{display = TRUE}, the function optimizes the
    encoding for display by removing default ignorable characters (soft
    hyphens, zero-width spaces, etc.) and placing zero-width spaces after
    wide emoji. When \code{output_utf8()} is \code{FALSE} the function
    escapes all non-ASCII characters and gives the same results on all
    platforms.
}
\value{
    A character object with the same attributes as \code{x} but with
    \code{Encoding} set to \code{"UTF-8"}.
}
\seealso{
    \code{\link{utf8_print}}.
}
\examples{
# the second element is encoded in latin-1, but declared as UTF-8
x <- c("fa\u00E7ile", "fa\xE7ile", "fa\xC3\xA7ile")
Encoding(x) <- c("UTF-8", "UTF-8", "bytes")

# encoding
utf8_encode(x)

# add style to the escapes
cat(utf8_encode("hello\nstyled\\\\world", escapes = "1"), "\n")
}
