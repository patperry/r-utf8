<!-- README.md is generated from README.Rmd. Please edit that file -->

# utf8

<!-- badges: start -->

[![rcc](https://github.com/patperry/r-utf8/workflows/rcc/badge.svg)](https://github.com/patperry/r-utf8/actions) [![Coverage Status](https://codecov.io/github/patperry/r-utf8/coverage.svg?branch=master "Code Coverage")](https://codecov.io/github/patperry/r-utf8?branch=master "Code Coverage") [![CRAN Status](https://www.r-pkg.org/badges/version/utf8 "CRAN Page")](https://cran.r-project.org/package=utf8 "CRAN Page") [![License](https://img.shields.io/badge/License-Apache%202.0-blue.svg "Apache License, Version 2.0")](https://www.apache.org/licenses/LICENSE-2.0.html "Apache License, Version 2.0") [![CRAN RStudio Mirror Downloads](https://cranlogs.r-pkg.org/badges/utf8 "CRAN Downloads")](https://cran.r-project.org/package=utf8 "CRAN Page")

<!-- badges: end -->

*utf8* is an R package for manipulating and printing UTF-8 text that fixes [multiple](https://twitter.com/ptrckprry/status/901494853758054401 "Windows enc2utf8 Bug") [bugs](https://twitter.com/ptrckprry/status/887732831161425920 "MacOS Emoji Printing") in R’s UTF-8 handling.

## Installation

### Stable version

*utf8* is [available on CRAN](https://cran.r-project.org/package=utf8 "CRAN Page"). To install the latest released version, run the following command in R:

<pre class='chroma'>
<span class='nf'><a href='https://rdrr.io/r/utils/install.packages.html'>install.packages</a></span><span class='o'>(</span><span class='s'>"utf8"</span><span class='o'>)</span></pre>

### Development version

To install the latest development version, run the following:

<pre class='chroma'>
<span class='nf'>devtools</span><span class='nf'>::</span><span class='nf'><a href='https://devtools.r-lib.org//reference/remote-reexports.html'>install_github</a></span><span class='o'>(</span><span class='s'>"patperry/r-utf8"</span><span class='o'>)</span></pre>

## Usage

<pre class='chroma'>
<span class='kr'><a href='https://rdrr.io/r/base/library.html'>library</a></span><span class='o'>(</span><span class='nv'><a href='https://github.com/patperry/r-utf8'>utf8</a></span><span class='o'>)</span></pre>

### Validate character data and convert to UTF-8

Use [`as_utf8()`](https://rdrr.io/pkg/utf8/man/as_utf8.html) to validate input text and convert to UTF-8 encoding. The function alerts you if the input text has the wrong declared encoding:

<pre class='chroma'>
<span class='c'># second entry is encoded in latin-1, but declared as UTF-8</span>
<span class='nv'>x</span> <span class='o'>&lt;-</span> <span class='nf'><a href='https://rdrr.io/r/base/c.html'>c</a></span><span class='o'>(</span><span class='s'>"fa\u00E7ile"</span>, <span class='s'>"fa\xE7ile"</span>, <span class='s'>"fa\xC3\xA7ile"</span><span class='o'>)</span>
<span class='nf'><a href='https://rdrr.io/r/base/Encoding.html'>Encoding</a></span><span class='o'>(</span><span class='nv'>x</span><span class='o'>)</span> <span class='o'>&lt;-</span> <span class='nf'><a href='https://rdrr.io/r/base/c.html'>c</a></span><span class='o'>(</span><span class='s'>"UTF-8"</span>, <span class='s'>"UTF-8"</span>, <span class='s'>"bytes"</span><span class='o'>)</span>
<span class='nf'><a href='https://rdrr.io/pkg/utf8/man/as_utf8.html'>as_utf8</a></span><span class='o'>(</span><span class='nv'>x</span><span class='o'>)</span> <span class='c'># fails</span>
<span class='c'>#&gt; Error in as_utf8(x): entry 2 has wrong Encoding; marked as "UTF-8" but leading byte 0xE7 followed by invalid continuation byte (0x69) at position 4</span>

<span class='c'># mark the correct encoding</span>
<span class='nf'><a href='https://rdrr.io/r/base/Encoding.html'>Encoding</a></span><span class='o'>(</span><span class='nv'>x</span><span class='o'>[</span><span class='m'>2</span><span class='o'>]</span><span class='o'>)</span> <span class='o'>&lt;-</span> <span class='s'>"latin1"</span>
<span class='nf'><a href='https://rdrr.io/pkg/utf8/man/as_utf8.html'>as_utf8</a></span><span class='o'>(</span><span class='nv'>x</span><span class='o'>)</span> <span class='c'># succeeds</span>
<span class='c'>#&gt; [1] "façile" "façile" "façile"</span></pre>

### Normalize data

Use [`utf8_normalize()`](https://rdrr.io/pkg/utf8/man/utf8_normalize.html) to convert to Unicode composed normal form (NFC). Optionally apply compatibility maps for NFKC normal form or case-fold.

<pre class='chroma'>
<span class='c'># three ways to encode an angstrom character</span>
<span class='o'>(</span><span class='nv'>angstrom</span> <span class='o'>&lt;-</span> <span class='nf'><a href='https://rdrr.io/r/base/c.html'>c</a></span><span class='o'>(</span><span class='s'>"\u00c5"</span>, <span class='s'>"\u0041\u030a"</span>, <span class='s'>"\u212b"</span><span class='o'>)</span><span class='o'>)</span>
<span class='c'>#&gt; [1] "Å" "Å" "Å"</span>
<span class='nf'><a href='https://rdrr.io/pkg/utf8/man/utf8_normalize.html'>utf8_normalize</a></span><span class='o'>(</span><span class='nv'>angstrom</span><span class='o'>)</span> <span class='o'>==</span> <span class='s'>"\u00c5"</span>
<span class='c'>#&gt; [1] TRUE TRUE TRUE</span>

<span class='c'># perform full Unicode case-folding</span>
<span class='nf'><a href='https://rdrr.io/pkg/utf8/man/utf8_normalize.html'>utf8_normalize</a></span><span class='o'>(</span><span class='s'>"Größe"</span>, map_case <span class='o'>=</span> <span class='kc'>TRUE</span><span class='o'>)</span>
<span class='c'>#&gt; [1] "grösse"</span>

<span class='c'># apply compatibility maps to NFKC normal form</span>
<span class='c'># (example from https://twitter.com/aprilarcus/status/367557195186970624)</span>
<span class='nf'><a href='https://rdrr.io/pkg/utf8/man/utf8_normalize.html'>utf8_normalize</a></span><span class='o'>(</span><span class='s'>"𝖸𝗈 𝐔𝐧𝐢𝐜𝐨𝐝𝐞 𝗅 𝗁𝖾𝗋𝖽 𝕌 𝗅𝗂𝗄𝖾 𝑡𝑦𝑝𝑒𝑓𝑎𝑐𝑒𝑠 𝗌𝗈 𝗐𝖾 𝗉𝗎𝗍 𝗌𝗈𝗆𝖾 𝚌𝚘𝚍𝚎𝚙𝚘𝚒𝚗𝚝𝚜 𝗂𝗇 𝗒𝗈𝗎𝗋 𝔖𝔲𝔭𝔭𝔩𝔢𝔪𝔢𝔫𝔱𝔞𝔯𝔶 𝔚𝔲𝔩𝔱𝔦𝔩𝔦𝔫𝔤𝔳𝔞𝔩 𝔓𝔩𝔞𝔫𝔢 𝗌𝗈 𝗒𝗈𝗎 𝖼𝖺𝗇 𝓮𝓷𝓬𝓸𝓭𝓮 𝕗𝕠𝕟𝕥𝕤 𝗂𝗇 𝗒𝗈𝗎𝗋 𝒇𝒐𝒏𝒕𝒔."</span>,
               map_compat <span class='o'>=</span> <span class='kc'>TRUE</span><span class='o'>)</span>
<span class='c'>#&gt; [1] "Yo Unicode l herd U like typefaces so we put some codepoints in your Supplementary Wultilingval Plane so you can encode fonts in your fonts."</span></pre>

### Print emoji

On some platforms (including MacOS), the R implementation of [`print()`](https://rdrr.io/r/base/print.html) uses an outdated version of the Unicode standard to determine which characters are printable. Use [`utf8_print()`](https://rdrr.io/pkg/utf8/man/utf8_print.html) for an updated print function:

<pre class='chroma'>
<span class='nf'><a href='https://rdrr.io/r/base/print.html'>print</a></span><span class='o'>(</span><span class='nf'><a href='https://rdrr.io/r/base/utf8Conversion.html'>intToUtf8</a></span><span class='o'>(</span><span class='m'>0x1F600</span> <span class='o'>+</span> <span class='m'>0</span><span class='o'>:</span><span class='m'>79</span><span class='o'>)</span><span class='o'>)</span> <span class='c'># with default R print function</span>
<span class='c'>#&gt; [1] "😀😁😂😃😄😅😆😇😈😉😊😋😌😍😎😏😐😑😒😓😔😕😖😗😘😙😚😛😜😝😞😟😠😡😢😣😤😥😦😧😨😩😪😫😬😭😮😯😰😱😲😳😴😵😶😷😸😹😺😻😼😽😾😿🙀🙁🙂🙃🙄🙅🙆🙇🙈🙉🙊🙋🙌🙍🙎🙏"</span>

<span class='nf'><a href='https://rdrr.io/pkg/utf8/man/utf8_print.html'>utf8_print</a></span><span class='o'>(</span><span class='nf'><a href='https://rdrr.io/r/base/utf8Conversion.html'>intToUtf8</a></span><span class='o'>(</span><span class='m'>0x1F600</span> <span class='o'>+</span> <span class='m'>0</span><span class='o'>:</span><span class='m'>79</span><span class='o'>)</span><span class='o'>)</span> <span class='c'># with utf8_print, truncates line</span>
<span class='c'>#&gt; [1] "😀​😁​😂​😃​😄​😅​😆​😇​😈​😉​😊​😋​😌​😍​😎​😏​😐​😑​😒​😓​😔​😕​😖​😗​😘​😙​😚​😛​😜​😝​😞​😟​😠​😡​😢​😣​😤​😥​😦​😧​😨​😩​😪​😫​…"</span>

<span class='nf'><a href='https://rdrr.io/pkg/utf8/man/utf8_print.html'>utf8_print</a></span><span class='o'>(</span><span class='nf'><a href='https://rdrr.io/r/base/utf8Conversion.html'>intToUtf8</a></span><span class='o'>(</span><span class='m'>0x1F600</span> <span class='o'>+</span> <span class='m'>0</span><span class='o'>:</span><span class='m'>79</span><span class='o'>)</span>, chars <span class='o'>=</span> <span class='m'>1000</span><span class='o'>)</span> <span class='c'># higher character limit</span>
<span class='c'>#&gt; [1] "😀​😁​😂​😃​😄​😅​😆​😇​😈​😉​😊​😋​😌​😍​😎​😏​😐​😑​😒​😓​😔​😕​😖​😗​😘​😙​😚​😛​😜​😝​😞​😟​😠​😡​😢​😣​😤​😥​😦​😧​😨​😩​😪​😫​😬​😭​😮​😯​😰​😱​😲​😳​😴​😵​😶​😷​😸​😹​😺​😻​😼​😽​😾​😿​🙀​🙁​🙂​🙃​🙄​🙅​🙆​🙇​🙈​🙉​🙊​🙋​🙌​🙍​🙎​🙏​"</span></pre>

## Citation

Cite *utf8* with the following BibTeX entry:

    @Manual{,
      title = {utf8: Unicode Text Processing},
      author = {Patrick O. Perry},
      year = {2018},
      note = {R package version 1.1.4},
      url = {https://github.com/patperry/r-utf8},
    }

## Contributing

The project maintainer welcomes contributions in the form of feature requests, bug reports, comments, unit tests, vignettes, or other code. If you’d like to contribute, either

-   fork the repository and submit a pull request

-   [file an issue](https://github.com/patperry/r-utf8/issues "Issues");

-   or contact the maintainer via e-mail.

This project is released with a [Contributor Code of Conduct](https://github.com/patperry/r-utf8/blob/master/CONDUCT.md "Contributor Code of Conduct"), and if you choose to contribute, you must adhere to its terms.
