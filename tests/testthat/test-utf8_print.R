context("utf8_print")

test_that("'utf8_print' can print unicode", {
    skip_on_os("windows")
    ctype <- switch_ctype("Unicode")
    on.exit(Sys.setlocale("LC_CTYPE", ctype))

    x <-  c("\u0100\u0101\u0102\u0103\u0104\u0105",
            "\u0106\u0107\u0108\u0109\u010a\u010b")

    expect_equal(capture_output(utf8_print(x)),
                 paste("[1] \"\u0100\u0101\u0102\u0103\u0104\u0105\"",
                       "\"\u0106\u0107\u0108\u0109\u010a\u010b\""))
})

test_that("'utf8_print' works with unnamed character vectors", {
    x <- as.character(1:100)

    expect_equal(capture_output(utf8_print(x)),
                 capture_output(print(x)))

    expect_equal(capture_output(utf8_print(x[1:96])),
                 capture_output(print(x[1:96])))

    expect_equal(capture_output(utf8_print(x[1:7])),
                 capture_output(print(x[1:7])))
})


test_that("'utf8_print' works with named character vectors", {
    x <- as.character(10 + 1:26)
    names(x) <- letters

    # left align names
    xr <- x
    names(xr) <- format(names(x), aligh="left", width = 4)
    actual <- strsplit(capture_output(utf8_print(x)), "\n")[[1]]
    expected <- strsplit(capture_output(print(xr)), "\n")[[1]]
    expect_equal(paste(actual, ""), expected)

    actual <- strsplit(capture_output(utf8_print(x[1:16])), "\n")[[1]]
    expected <- strsplit(capture_output(print(xr[1:16])), "\n")[[1]]
    expect_equal(paste(actual, ""), expected)

    actual <- strsplit(capture_output(utf8_print(x[1:4])), "\n")[[1]]
    expected <- strsplit(capture_output(print(xr[1:4])), "\n")[[1]]
    expect_equal(paste(actual, ""), expected)
})


test_that("'utf8_print' can use the 'max' argument for unnamed vectors", {
    x <- as.character(1:100)

    expect_equal(capture_output(utf8_print(x, max = 0), width = 80),
                 " [ reached getOption(\"max.print\") -- omitted 100 entries ]")

    expect_equal(capture_output(utf8_print(x, max = 100), width = 80),
                 capture_output(utf8_print(x), width = 80))

    lines <- strsplit(capture_output(utf8_print(x, max = 20), width = 80),
                      "\n")[[1]]
    expect_equal(length(lines), 3)
    expect_equal(lines[[3]],
                 " [ reached getOption(\"max.print\") -- omitted 80 entries ]")
})


test_that("'utf8_print' can use the 'max' argument for named vectors", {
    x <- as.character(1:260)
    names(x) <- rep(letters, 10)

    expect_equal(capture_output(utf8_print(x, max = 0), width = 80),
                 " [ reached getOption(\"max.print\") -- omitted 260 entries ]")

    expect_equal(capture_output(utf8_print(x, max = 260), width = 80),
                 capture_output(utf8_print(x), width = 80))

    lines <- strsplit(capture_output(utf8_print(x, max = 20), width = 80),
                      "\n")[[1]]
    expect_equal(length(lines), 5)
    expect_equal(lines[[5]],
                 " [ reached getOption(\"max.print\") -- omitted 240 entries ]")
})


test_that("'utf8_print' can print empty vectors", {
    expect_equal(capture_output(utf8_print(character())), "character(0)")
    expect_equal(capture_output(utf8_print(array(character(), 0))), "character(0)")
})


test_that("'utf8_print' can print matrices", {
    x1 <- matrix(letters, 13, 2)

    x2 <- matrix(letters, 13, 2)
    rownames(x2) <- LETTERS[1:13]

    x3 <- matrix(letters, 13, 2)
    colnames(x3) <- c("x", "y")

    x4 <- matrix(letters, 13, 2)
    rownames(x4) <- LETTERS[1:13]
    colnames(x4) <- c("x", "y")

    expect_equal(capture_output(utf8_print(x1)),
                 capture_output(print(x1)))

    expect_equal(capture_output(utf8_print(x2)),
                 capture_output(print(x2)))

    expect_equal(capture_output(utf8_print(x3)),
                 capture_output(print(x3)))

    expect_equal(capture_output(utf8_print(x4)),
                 capture_output(print(x4)))
})


test_that("'utf8_print' can print empty matrices", {
    x1 <- matrix(character(), 10, 0)
    x2 <- matrix(character(), 0, 10)
    x3 <- matrix(character(), 0, 0)

    expect_equal(paste0("     \n", capture_output(utf8_print(x1))),
                 capture_output(print(x1)))

    expect_equal(paste0("     ", capture_output(utf8_print(x2))),
                 capture_output(print(x2)))

    expect_equal(capture_output(utf8_print(x3)),
                 capture_output(print(x3)))
})


test_that("'utf8_print' can print arrays", {
    x <- array(as.character(1:24), c(2,3,4,5))

    expect_equal(capture_output(utf8_print(x)),
                 capture_output(print(x)))

    x2 <- x
    dimnames(x2) <- list(letters[1:2], letters[3:5], letters[6:9],
                         letters[10:14])

    expect_equal(capture_output(utf8_print(x2)),
                 capture_output(print(x2)))
})


test_that("'utf8_print' can print empty arrays", {
    expect_equal(capture_output(utf8_print(array(character(), c(2,3,0)))),
                 "<2 x 3 x 0 array>")

    expect_equal(capture_output(utf8_print(array(character(), c(2,0,3)))),
                 "<2 x 0 x 3 array>")

    expect_equal(capture_output(utf8_print(array(character(), c(0,2,3)))),
                 "<0 x 2 x 3 array>")
})


test_that("'utf8_print' can print quotes", {
    expect_equal(capture_output(utf8_print('"')),
                 capture_output(print('"')))

    expect_equal(capture_output(utf8_print('"', quote = FALSE)),
                 capture_output(print('"', quote = FALSE)))
})


test_that("'utf8_print' can handle NA", {
    expect_equal(capture_output(utf8_print(NA_character_)),
                 capture_output(print(NA_character_)))
    expect_equal(capture_output(utf8_print(NA_character_, quote = FALSE)),
                 capture_output(print(NA_character_, quote = FALSE)))
})


test_that("'utf8_print' can handle NA names", {
    x <- matrix("hello", 1, 1, dimnames=list(NA,NA))
    expect_equal(capture_output(utf8_print(x)),
                 capture_output(print(x)))
    expect_equal(capture_output(utf8_print(x, na.print = "foo")),
                 capture_output(print(x, na.print = "foo")))
})


test_that("'utf8_print' can right justify", {
    x <- matrix(c("a", "ab", "abc"), 3, 1,
                dimnames = list(c("1", "2", "3"), "ch"))
    expect_equal(capture_output(utf8_print(x, quote = FALSE, right = TRUE)),
                 capture_output(print(x, quote = FALSE, right = TRUE)))
    expect_equal(capture_output(utf8_print(x, quote = TRUE, right = TRUE)),
                 capture_output(print(x, quote = TRUE, right = TRUE)))
})
