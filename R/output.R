#  Copyright 2017 Patrick O. Perry.
#
#  Licensed under the Apache License, Version 2.0 (the "License");
#  you may not use this file except in compliance with the License.
#  You may obtain a copy of the License at
#
#      http://www.apache.org/licenses/LICENSE-2.0
#
#  Unless required by applicable law or agreed to in writing, software
#  distributed under the License is distributed on an "AS IS" BASIS,
#  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
#  See the License for the specific language governing permissions and
#  limitations under the License.


output_ansi <- function()
{
    # tty?
    if (isatty(stdout())) {
        # and not Windows GUI?
        return(.Platform$GUI != "Rgui")
    }

    # RStudio 1.1 or later with color enabled?
    if (!is.na(as.numeric(Sys.getenv("RSTUDIO_CONSOLE_COLOR")))) {
        # and output is stdout?
        return(stdout() == 1)
    }

    FALSE
}


output_utf8 <- function()
{
    # ASCII-only character locale?
    if (Sys.getlocale("LC_CTYPE") == "C") {
        return(FALSE)
    }

    # UTF-8 locale?
    if (l10n_info()$`UTF-8`) {
        return(TRUE)
    }

    # Windows?
    if (.Platform$OS.type == "windows") {
        # This isn't really the case, but there's no way to set the
        # locale to UTF-8 on Windows. In RGui and RStudio, UTF-8 is
        # always supported on stdout(); output through connections
        # gets translated through the native locale.
        return(TRUE)
    }

    FALSE
}
