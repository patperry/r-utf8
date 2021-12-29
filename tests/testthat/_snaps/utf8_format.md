# 'format' can handle long text in Unicode locale: UTF-8 is TRUE

    Code

# 'format' can handle long text in Unicode locale: UTF-8 is FALSE

    Code
      raw <- c(NA, "", "a", "ab", "foo", "food", "short text", "<U+6027>",
        "<U+6027><U+6027>", "<U+6027>?")
      Encoding(raw) <- "UTF-8"
      utf8_format(raw, chars = 2, justify = "none", na.print = "NA")
    Output
       [1] "NA"  ""    "a"   "ab"  "foâ€¦" "foâ€¦" "shâ€¦" "<Uâ€¦" "<Uâ€¦" "<Uâ€¦"
    Code
      utf8_format(raw, chars = 2, justify = "left", na.print = "NA")
    Output
       [1] "NA " "   " "a  " "ab " "foâ€¦" "foâ€¦" "shâ€¦" "<Uâ€¦" "<Uâ€¦" "<Uâ€¦"
    Code
      utf8_format(raw, chars = 2, justify = "centre", na.print = "NA")
    Output
       [1] "NA " "   " " a " "ab " "foâ€¦" "foâ€¦" "shâ€¦" "<Uâ€¦" "<Uâ€¦" "<Uâ€¦"
    Code
      utf8_format(raw, chars = 2, justify = "right", na.print = "NA")
    Output
       [1] " NA" "   " "  a" " ab" "â€¦oo" "â€¦od" "â€¦xt" "â€¦7>" "â€¦7>" "â€¦>?"

