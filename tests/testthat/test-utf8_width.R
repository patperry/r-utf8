test_that("'utf8_width' computes widths correctly", {
  local_ctype("UTF-8")

  expect_equal(
    utf8_width(c("hello", "\u200b", "\u22ee", "\u6027"),
      encode = FALSE
    ),
    c(5, 0, 1, 2)
  )
})


test_that("'utf8_width' computes widths for extended unicode correctly", {
  local_ctype("UTF-8")
  skip_on_os("windows") # no extended Unicode

  expect_equal(utf8_width(intToUtf8(0x1f642), encode = FALSE), 2)
})




test_that("'utf8_width' gives NA for non-ASCII in C locale", {
  x <- c("hello", "\u200b", "\u22ee", "\u6027", intToUtf8(0x1f642))

  # https://github.com/r-lib/testthat/issues/1285
  with_ctype("C", {
    width <- utf8_width(x, encode = FALSE)
  })

  expect_equal(width, c(5, NA, NA, NA, NA))
})


test_that("'utf8_width' keeps names", {
  expect_equal(
    utf8_width(c(a = "hello", b = "you")),
    c(a = 5, b = 3)
  )
})


test_that("'utf8_width' gives NA for control characters", {
  expect_equal(
    utf8_width(c("\u0001", "\a", "new\nline"),
      encode = FALSE
    ),
    c(NA_integer_, NA_integer_, NA_integer_)
  )
})


test_that("'utf8_width' gives NA for invalid data", {
  x <- c("a", "b", "\xff", "abc\xfe")
  Encoding(x) <- "UTF-8"
  expect_equal(utf8_width(x, encode = FALSE), c(1, 1, NA, NA))
})


test_that("'utf8_width' gives width 1 for quotes", {
  expect_equal(utf8_width("\"", encode = TRUE), 1)
  expect_equal(utf8_width("\"", encode = FALSE), 1)
})


test_that("'utf8_width' gives correct with in C locale", {
  x <- intToUtf8(c(0x1F487, 0x200D, 0x2642, 0xFE0F))

  # https://github.com/r-lib/testthat/issues/1285
  with_ctype("C", {
    width <- utf8_width(x)
    encoded <- utf8_encode(x)
  })

  expect_equal(width, nchar(encoded))
})
