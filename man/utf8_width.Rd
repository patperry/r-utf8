\name{utf8_width}
\title{Measure the Character String Width}
\alias{utf8_width}
\description{
Compute the display widths of the elements of a character object.
}
\usage{
utf8_width(x, encode = TRUE, quote = FALSE, utf8 = NULL)
}
\arguments{
    \item{x}{character object.}

    \item{encode}{whether to encode the object before measuring its
        width.}

    \item{quote}{whether to quote the object before measuring its
        width.}

    \item{utf8}{logical scalar indicating whether to determine widths
        assuming a UTF-8 capable display (ASCII-only otherwise), or
        \code{NULL} to format for output capabilities as determined
        by \code{output_utf8()}.}
}
\details{
    \code{utf8_width} returns the printed widths of the elements of
    a character object on a UTF-8 device (or on an ASCII device
    when \code{output_utf8()} is \code{FALSE}), when printed with
    \code{utf8_print}. If the string is not printable on the device,
    for example if it contains a control code like \code{"\n"}, then
    the result is \code{NA}. If \code{encode = TRUE}, the default,
    then the function returns the widths of the encoded elements
    via \code{utf8_encode}); otherwise, the function returns the
    widths of the original elements.
}
\value{
    An integer object,  with the same \code{names}, \code{dim}, and
    \code{dimnames} as \code{x}.
}
\seealso{
    \code{\link{utf8_print}}.
}
\examples{
# the second element is encoded in latin-1, but declared as UTF-8
x <- c("fa\u00E7ile", "fa\xE7ile", "fa\xC3\xA7ile")
Encoding(x) <- c("UTF-8", "UTF-8", "bytes")

# get widths
utf8_width(x)
utf8_width(x, encode = FALSE)
utf8_width('"')
utf8_width('"', quote = TRUE)
}
