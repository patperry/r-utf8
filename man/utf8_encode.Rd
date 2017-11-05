\name{utf8_encode}
\title{Encode Character Object as for UTF-8 Printing}
\alias{utf8_encode}
\description{
    Escape the strings in a character object, optionally adding
    quotes or spaces, adjusting the width for display.
}
\usage{
utf8_encode(x, width = 0L, quote = FALSE, justify = "left", display = FALSE,
            style = FALSE)
}
\arguments{
    \item{x}{character object.}

    \item{width}{integer giving the minimum field width; specify
        \code{NULL} or \code{NA} for no minimum.}

    \item{quote}{logical scalar indicating whether to surround results
        with double-quotes and escape internal double-quotes.}

    \item{justify}{justification; one of \code{"left"}, \code{"right"},
        \code{"centre"}, or \code{"none"}. Can be abbreviated.}

    \item{display}{logical scalar indicating whether to optimize the
        encoding for display, not byte-for-byte data transmission.}

    \item{style}{logical scalar indicating whether to insert ANSI terminal
        escape codes to display backslash escapes in faint (decreased
        intensity) style.}
}
\details{
    \code{utf8_encode} encodes a character object for printing on a UTF-8
    device by escaping controls characters and other non-printable
    characters. When \code{display = TRUE}, the function optimizes the
    encoding for display by removing default ignorable characters (soft
    hyphens, zero-width spaces, etc.) and placing zero-width spaces after
    wide emoji. When \code{LC_CTYPE = "C"}, the function escapes all
    non-ASCII characters and gives the same results on all platforms.
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
}
