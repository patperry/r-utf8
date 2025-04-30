<!-- NEWS.md is maintained by https://fledge.cynkra.com, contributors should not edit this file -->

# utf8 1.2.4.9000 (2025-04-30)

## Chore

- Replace `[v]sprintf()` with `[v]snprintf()` (#67).

- Add direct include for snprintf (@MichaelChirico, #43).

## Documentation

- Use roxygen2 (#68, #69).


# utf8 1.2.3.9022 (2025-04-30)

## Chore

- Add direct include for snprintf (@MichaelChirico, #43).

- Auto-update from GitHub Actions.

  Run: https://github.com/patperry/r-utf8/actions/runs/14636196819

## Continuous integration

- Permissions, better tests for missing suggests, lints (#66).

- Only fail covr builds if token is given (#65).

- Always use `_R_CHECK_FORCE_SUGGESTS_=false` (#64).

- Correct installation of xml2 (#63).

- Explain (#62).

- Add xml2 for covr, print testthat results (#61).

- Fix (#60).

- Sync (#59).


# utf8 1.2.3.9021 (2024-12-09)

## Continuous integration

- Avoid failure in fledge workflow if no changes (#58).


# utf8 1.2.3.9020 (2024-12-08)

## Continuous integration

- Fetch tags for fledge workflow to avoid unnecessary NEWS entries (#57).


# utf8 1.2.3.9019 (2024-12-07)

## Continuous integration

- Use larger retry count for lock-threads workflow (#56).


# utf8 1.2.3.9018 (2024-11-28)

## Continuous integration

- Ignore errors when removing pkg-config on macOS (#55).


# utf8 1.2.3.9017 (2024-11-27)

## Continuous integration

- Explicit permissions (#54).


# utf8 1.2.3.9016 (2024-11-26)

## Continuous integration

- Use styler from main branch (#53).


# utf8 1.2.3.9015 (2024-11-25)

## Continuous integration

- Need to install R on Ubuntu 24.04 (#52).

- Use Ubuntu 24.04 and styler PR (#50).


# utf8 1.2.3.9014 (2024-11-22)

## Continuous integration

  - Correctly detect branch protection (#49).


# utf8 1.2.3.9013 (2024-11-18)

## Continuous integration

  - Use stable pak (#48).


# utf8 1.2.3.9012 (2024-11-11)

## Continuous integration

  - Trigger run (#47).
    
      - ci: Trigger run
    
      - ci: Latest changes


# utf8 1.2.3.9011 (2024-10-28)

## Continuous integration

  - Use pkgdown branch (#46).
    
      - ci: Use pkgdown branch
    
      - ci: Updates from duckdb
    
      - ci: Trigger run


# utf8 1.2.3.9010 (2024-09-15)

## Continuous integration

  - Install via R CMD INSTALL ., not pak (#44).
    
      - ci: Install via R CMD INSTALL ., not pak
    
      - ci: Bump version of upload-artifact action


# utf8 1.2.3.9009 (2024-08-31)

## Chore

  - Auto-update from GitHub Actions.
    
    Run: https://github.com/patperry/r-utf8/actions/runs/10425486174

  - Auto-update from GitHub Actions.
    
    Run: https://github.com/patperry/r-utf8/actions/runs/10200110873

  - Auto-update from GitHub Actions.
    
    Run: https://github.com/patperry/r-utf8/actions/runs/9727975031

  - Auto-update from GitHub Actions.
    
    Run: https://github.com/patperry/r-utf8/actions/runs/9691616880

## Continuous integration

  - Install local package for pkgdown builds.

  - Improve support for protected branches with fledge.

  - Improve support for protected branches, without fledge.

  - Sync with latest developments.

  - Use v2 instead of master.

  - Inline action.

  - Use dev roxygen2 and decor.

  - Fix on Windows, tweak lock workflow.

  - Avoid checking bashisms on Windows.

  - Better commit message.

  - Bump versions, better default, consume custom matrix.

  - Recent updates.


# utf8 1.2.3.9008 (2024-01-24)

- Internal changes only.


# utf8 1.2.3.9007 (2024-01-15)

- Internal changes only.


# utf8 1.2.4 (2023-10-16)

- Fix compatibility with macOS 14 (#39).


# utf8 1.2.3 (2023-01-30)

## Features

- Support Unicode 14.

## Chore

- Update maintainer e-mail address.

- Fix compiler warnings (@Antonov548, #37).


# utf8 1.2.2 (2021-07-24)

- Reenable all tests.
- `utf8_width()` now reports correct widths for narrow emojis (#9).


# utf8 1.2.1 (2021-03-10)

- Use Unicode and Emoji standards version 13.0 via upgrade to latest `utf8lite`.
- Silence test on macOS.


# utf8 1.1.4 (2018-05-24)

## BUG FIXES

- Fix build on Solaris (#7, reported by @krlmlr).

- Fix rendering of emoji ZWJ sequences like `"\U1F469\U200D\U2764\UFE0F\U200D\U1F48B\U200D\U1F469"`.


# utf8 1.1.3 (2018-01-03)

## MINOR IMPROVEMENTS

- Make `output_utf8()` always return `TRUE` on Windows, so that characters in the user's native locale don't get escaped by `utf8_encode()`. The downside of this change is that on Windows, `utf8_width()` reports the wrong values for characters outside the user's locale when `stdout()` is redirected by `knitr` or another process.

- When truncating long strings strings via `utf8_format()`, use an ellipsis that is printable in the user's native locale (`"\u2026" or `"..."`).


# utf8 1.1.2 (2017-12-14)

## BUG FIXES

- Fix bug in `utf8_format()` with non-`NULL` `width` argument.


# utf8 1.1.1 (2017-11-28)

## BUG FIXES

- Fix PROTECT bug in `as_utf8()`.


# utf8 1.1.0 (2017-11-20)

## NEW FEATURES

- Added `output_ansi()` and `output_utf8()` functions to test for output capabilities.

## MINOR IMPROVEMENTS

- Add `utf8` argument to `utf8_encode()`, `utf8_format()`, `utf8_print()`, and `utf8_width()` for precise control over assumed output capabilities; defaults to the result of `output_utf8()`.

- Add ability to style backslash escapes with the `escapes` arguments to `utf8_encode()` and `utf8_print()`. Switch from "faint" styling to no styling by default.

- Slightly reword error messages for `as_utf8()`.

- Fix (spurious) `rchk` warnings.

## BUG FIXES

- Fix bug in `utf8_width()` determining width of non-ASCII strings when `LC_CTYPE=C`.

## DEPRECATED AND DEFUNCT

- No longer export the C version of `as_utf8()` (the R version is still present).


# utf8 1.0.0 (2017-11-06)

## NEW FEATURES

- Split off functions `as_utf8()`, `utf8_valid()`, `utf8_normalize()`, `utf8_encode()`, `utf8_format()`, `utf8_print()`, and `utf8_width()` from [corpus][corpus] package.

- Added special handling for Unicode grapheme clusters in formatting and width measurement functions.

- Added ANSI styling to escape sequences.

- Added ability to style row and column names in `utf8_print()`.


[corpus]: http://corpustext.com/ "corpus: Text Corpus Analysis"
