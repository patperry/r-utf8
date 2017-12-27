\name{output_utf8}
\title{Output Capabilities}
\alias{output_ansi}
\alias{output_utf8}
\description{
    Test whether the output connection has ANSI style escape support
    or UTF-8 support.
}
\usage{
output_ansi()

output_utf8()
}
\details{
\code{output_ansi} tests whether the output connection supports ANSI
style escapes. This is \code{TRUE} if the connection is a terminal
and not the Windows GUI. Otherwise, it is true if running in
RStudio 1.1 or later with ANSI escapes enabled, provided \code{stdout()}
has not been redirected to another connection by \code{sink()}.

\code{output_utf8} tests whether the output connection supports UTF-8. For
most platforms \code{l10n_info()$`UTF-8`} gives this information, but this
does not give an accurate result for Windows GUIs. To work around this,
we proceed as follows:
\itemize{
  \item if the character locale (\code{LC_CTYPE}) is \code{"C"}, then
      the result is \code{FALSE};

  \item otherwise, if \code{l10n_info()$`UTF-8`} is \code{TRUE},
      then the result is \code{TRUE};

  \item if running on Windows, then the result is \code{TRUE};
    
  \item in all other cases the result is \code{FALSE}.
}
Strictly speaking, UTF-8 support is always available on Windows GUI,
but only a subset of UTF-8 is available (defined by the current character
locale) when the output is redirected by \code{knitr} or another process.
Unfortunately, it is impossible to set the character locale to UTF-8 on
Windows. Further, the \code{utf8} package only handles two character
locales: C and UTF-8.  To get around this, on Windows, we treat all
non-C locales on that platform as UTF-8. This liberal approach means that
characters in the user's locale never get escaped; others will get output
as \code{<U+XXXX>}, with incorrect values for \code{utf8_width}.
}
\value{
A logical scalar indicating whether the output connection supports the
given capability.
}
\seealso{
    \code{\link{.Platform}}, \code{\link{isatty}}, \code{\link{l10n_info}},
    \code{\link{Sys.getlocale}}
}
\examples{
# test whether ANSI style escapes or UTF-8 output are supported
cat("ANSI:", output_ansi(), "\n")
cat("UTF8:", output_utf8(), "\n")

# switch to C locale
Sys.setlocale("LC_CTYPE", "C")
cat("ANSI:", output_ansi(), "\n")
cat("UTF8:", output_utf8(), "\n")

# switch to native locale
Sys.setlocale("LC_CTYPE", "")

tmp <- tempfile()
sink(tmp) # redirect output to a file
cat("ANSI:", output_ansi(), "\n")
cat("UTF8:", output_utf8(), "\n")
sink() # restore stdout

# inspect the output
readLines(tmp)
}
