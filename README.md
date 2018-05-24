<!-- README.md is generated from README.Rmd. Please edit that file -->



utf8
====

[![Build Status (Linux)][travis-badge]][travis]
[![Build Status (Windows)][appveyor-badge]][appveyor]
[![Coverage Status][codecov-badge]][codecov]
[![CRAN Status][cran-badge]][cran]
[![License][apache-badge]][apache]
[![CRAN RStudio Mirror Downloads][cranlogs-badge]][cran]


*utf8* is an R package for manipulating and printing UTF-8 text that fixes
[multiple][windows-enc2utf8] [bugs][emoji-print] in R's UTF-8 handling.


Installation
------------

### Stable version

*utf8* is [available on CRAN][cran]. To install the latest released version,
run the following command in R:

```r
install.packages("utf8")
```

### Development version

To install the latest development version, run the following:

```r
tmp <- tempfile()
system2("git", c("clone", "--recursive", shQuote("https://github.com/patperry/r-utf8.git"), shQuote(tmp)))
devtools::install(tmp)
```

Note that *utf8* uses a git submodule, so you cannot use
`devtools::install_github`.


Usage
-----

### Validate character data and convert to UTF-8

Use `as_utf8` to validate input text and convert to UTF-8 encoding. The
function alerts you if the input text has the wrong declared encoding:


```r
# second entry is encoded in latin-1, but declared as UTF-8
x <- c("fa\u00E7ile", "fa\xE7ile", "fa\xC3\xA7ile")
Encoding(x) <- c("UTF-8", "UTF-8", "bytes")
as_utf8(x) # fails
#> Error in as_utf8(x): entry 2 has wrong Encoding; marked as "UTF-8" but leading byte 0xE7 followed by invalid continuation byte (0x69) at position 4

# mark the correct encoding
Encoding(x[2]) <- "latin1"
as_utf8(x) # succeeds
#> [1] "façile" "façile" "façile"
```

### Normalize data

Use `utf8_normalize` to convert to Unicode composed normal form (NFC).
Optionally apply compatibility maps for NFKC normal form or case-fold.


```r
# three ways to encode an angstrom character
(angstrom <- c("\u00c5", "\u0041\u030a", "\u212b"))
#> [1] "Å" "Å" "Å"
utf8_normalize(angstrom) == "\u00c5"
#> [1] TRUE TRUE TRUE

# perform full Unicode case-folding
utf8_normalize("Größe", map_case = TRUE)
#> [1] "grösse"

# apply compatibility maps to NFKC normal form
# (example from https://twitter.com/aprilarcus/status/367557195186970624)
utf8_normalize("𝖸𝗈 𝐔𝐧𝐢𝐜𝐨𝐝𝐞 𝗅 𝗁𝖾𝗋𝖽 𝕌 𝗅𝗂𝗄𝖾 𝑡𝑦𝑝𝑒𝑓𝑎𝑐𝑒𝑠 𝗌𝗈 𝗐𝖾 𝗉𝗎𝗍 𝗌𝗈𝗆𝖾 𝚌𝚘𝚍𝚎𝚙𝚘𝚒𝚗𝚝𝚜 𝗂𝗇 𝗒𝗈𝗎𝗋 𝔖𝔲𝔭𝔭𝔩𝔢𝔪𝔢𝔫𝔱𝔞𝔯𝔶 𝔚𝔲𝔩𝔱𝔦𝔩𝔦𝔫𝔤𝔳𝔞𝔩 𝔓𝔩𝔞𝔫𝔢 𝗌𝗈 𝗒𝗈𝗎 𝖼𝖺𝗇 𝓮𝓷𝓬𝓸𝓭𝓮 𝕗𝕠𝕟𝕥𝕤 𝗂𝗇 𝗒𝗈𝗎𝗋 𝒇𝒐𝒏𝒕𝒔.",
               map_compat = TRUE)
#> [1] "Yo Unicode l herd U like typefaces so we put some codepoints in your Supplementary Wultilingval Plane so you can encode fonts in your fonts."
```

### Print emoji

On some platforms (including MacOS), the R implementation of `print` uses an
outdated version of the Unicode standard to determine which characters are
printable. Use `utf8_print` for an updated print function:


```r
print(intToUtf8(0x1F600 + 0:79)) # with default R print function
#> [1] "\U0001f600\U0001f601\U0001f602\U0001f603\U0001f604\U0001f605\U0001f606\U0001f607\U0001f608\U0001f609\U0001f60a\U0001f60b\U0001f60c\U0001f60d\U0001f60e\U0001f60f\U0001f610\U0001f611\U0001f612\U0001f613\U0001f614\U0001f615\U0001f616\U0001f617\U0001f618\U0001f619\U0001f61a\U0001f61b\U0001f61c\U0001f61d\U0001f61e\U0001f61f\U0001f620\U0001f621\U0001f622\U0001f623\U0001f624\U0001f625\U0001f626\U0001f627\U0001f628\U0001f629\U0001f62a\U0001f62b\U0001f62c\U0001f62d\U0001f62e\U0001f62f\U0001f630\U0001f631\U0001f632\U0001f633\U0001f634\U0001f635\U0001f636\U0001f637\U0001f638\U0001f639\U0001f63a\U0001f63b\U0001f63c\U0001f63d\U0001f63e\U0001f63f\U0001f640\U0001f641\U0001f642\U0001f643\U0001f644\U0001f645\U0001f646\U0001f647\U0001f648\U0001f649\U0001f64a\U0001f64b\U0001f64c\U0001f64d\U0001f64e\U0001f64f"

utf8_print(intToUtf8(0x1F600 + 0:79)) # with utf8_print, truncates line
#> [1] "😀​😁​😂​😃​😄​😅​😆​😇​😈​😉​😊​😋​😌​😍​😎​😏​😐​😑​😒​😓​😔​😕​😖​😗​😘​😙​😚​😛​😜​😝​😞​😟​😠​😡​😢​😣​😤​😥​😦​😧​😨​😩​😪​😫​…"

utf8_print(intToUtf8(0x1F600 + 0:79), chars = 1000) # higher character limit
#> [1] "😀​😁​😂​😃​😄​😅​😆​😇​😈​😉​😊​😋​😌​😍​😎​😏​😐​😑​😒​😓​😔​😕​😖​😗​😘​😙​😚​😛​😜​😝​😞​😟​😠​😡​😢​😣​😤​😥​😦​😧​😨​😩​😪​😫​😬​😭​😮​😯​😰​😱​😲​😳​😴​😵​😶​😷​😸​😹​😺​😻​😼​😽​😾​😿​🙀​🙁​🙂​🙃​🙄​🙅​🙆​🙇​🙈​🙉​🙊​🙋​🙌​🙍​🙎​🙏​"
```


Citation
--------

Cite *utf8* with the following BibTeX entry:


```
@Manual{,
  title = {utf8: Unicode Text Processing},
  author = {Patrick O. Perry},
  year = {2018},
  note = {R package version 1.1.4},
  url = {https://github.com/patperry/r-utf8},
}
```


Contributing
------------

The project maintainer welcomes contributions in the form of feature requests,
bug reports, comments, unit tests, vignettes, or other code.  If you'd like to
contribute, either

 + fork the repository and submit a pull request (note the nonstandard
   instructions for [building from source][building]);

 + [file an issue][issues];

 + or contact the maintainer via e-mail.

This project is released with a [Contributor Code of Conduct][conduct],
and if you choose to contribute, you must adhere to its terms.


[apache]: https://www.apache.org/licenses/LICENSE-2.0.html "Apache License, Version 2.0"
[apache-badge]: https://img.shields.io/badge/License-Apache%202.0-blue.svg "Apache License, Version 2.0"
[appveyor]: https://ci.appveyor.com/project/patperry/r-utf8/branch/master "Continuous Integration (Windows)"
[appveyor-badge]: https://ci.appveyor.com/api/projects/status/github/patperry/r-utf8?branch=master&svg=true "Continuous Inegration (Windows)"
[building]: #development-version "Building from Source"
[codecov]: https://codecov.io/github/patperry/r-utf8?branch=master "Code Coverage"
[codecov-badge]: https://codecov.io/github/patperry/r-utf8/coverage.svg?branch=master "Code Coverage"
[conduct]: https://github.com/patperry/r-utf8/blob/master/CONDUCT.md "Contributor Code of Conduct"
[cran]: https://cran.r-project.org/package=utf8 "CRAN Page"
[cran-badge]: http://www.r-pkg.org/badges/version/utf8 "CRAN Page"
[cranlogs-badge]: http://cranlogs.r-pkg.org/badges/utf8 "CRAN Downloads"
[emoji-print]: https://twitter.com/ptrckprry/status/887732831161425920 "MacOS Emoji Printing"
[issues]: https://github.com/patperry/r-utf8/issues "Issues"
[travis]: https://travis-ci.org/patperry/r-utf8 "Continuous Integration (Linux)"
[travis-badge]: https://api.travis-ci.org/patperry/r-utf8.svg?branch=master "Continuous Integration (Linux)"
[windows-enc2utf8]: https://twitter.com/ptrckprry/status/901494853758054401 "Windows enc2utf8 Bug"
