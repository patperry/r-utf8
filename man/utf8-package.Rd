\name{utf8-package}
\alias{utf8-package}
\alias{utf8}
\docType{package}
\title{
The utf8 Package
}
\description{
UTF-8 Text Processing
}
\details{
Functions for manipulating and printing UTF-8 encoded text:

\itemize{
  \item \code{\link{as_utf8}} attempts to convert character data to UTF-8,
    throwing an error if the data is invalid;

  \item \code{\link{utf8_valid}} tests whether character data is valid
    according to its declared encoding;

  \item \code{\link{utf8_normalize}} converts text to Unicode composed
    normal form (NFC), optionally applying case-folding and compatibility
    maps;

  \item \code{\link{utf8_encode}} encodes a character string, escaping all
    control characters, so that it can be safely printed to the screen;

  \item \code{\link{utf8_format}} formats a character vector by truncating
    to a specified character width limit or by left, right, or center
    justifying;

  \item \code{\link{utf8_print}} prints UTF-8 character data to the screen;

  \item \code{\link{utf8_width}} measures the display width of UTF-8 character
    strings (many emoji and East Asian characters are twice as wide as other
    characters);

  \item \code{\link{output_ansi}} and \code{\link{output_utf8}} test for
    the output connections capabilities.
}

For a complete list of functions, use \code{library(help = "utf8")}.
}
\author{
Patrick O. Perry
}
\keyword{ package }
