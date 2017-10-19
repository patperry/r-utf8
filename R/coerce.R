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


as_character_scalar <- function(name, value, utf8 = TRUE) {
  if (is.null(value)) {
    return(NULL)
  }
  value <- as_character_vector(name, value, utf8)
  if (length(value) != 1) {
    stop(sprintf("'%s' must be a scalar character string", name))
  }
  value
}


as_character_vector <- function(name, value, utf8 = TRUE) {
  if (!(is.null(value) || is.character(value) || is.factor(value) ||
    is_corpus_text(value) || all(is.na(value)))) {
    stop(sprintf("'%s' must be text, a character vector, or NULL", name))
  }
  if (is.null(value)) {
    return(NULL)
  }
  value <- as.character(value)
  if (utf8) {
    value <- as_utf8(value)
  }
  value
}


as_connector <- function(value) {
  value <- as_character_scalar("connector", value)
  .Call(C_as_text_filter_connector, value)
}


as_double_scalar <- function(name, value, allow_null = FALSE) {
  if (is.null(value)) {
    if (allow_null) {
      return(NULL)
    } else {
      stop(sprintf("'%s' cannot be NULL", name))
    }
  }

  if (length(value) != 1) {
    stop(sprintf("'%s' must have length 1", name))
  }

  if (!(is.numeric(value) && !is.nan(value) && !is.na(value))) {
    stop(sprintf(
      "'%s' must be a numeric value%s", name,
      if (allow_null) " (or NULL)" else ""
    ))
  }

  as.double(value)
}


as_enum <- function(name, value, choices) {
  if (!(is.character(value) && length(value) == 1 && !is.na(value))) {
    stop(sprintf("'%s' must be a character string", name))
  }

  i <- pmatch(value, choices, nomatch = 0)
  if (all(i == 0)) {
    stop(
      sprintf("'%s' must be one of the following: ", name),
      paste(dQuote(choices), collapse = ", ")
    )
  }
  i <- i[i > 0]
  choices[[i]]
}


as_filter <- function(name, filter) {
  if (is.null(filter)) {
    return(NULL)
  }

  if (!is.list(filter)) {
    stop(sprintf("'%s' must be a text filter, list, or NULL", name))
  }
  filter <- unclass(filter)
  keys <- names(filter)

  props <- names(text_filter())
  unknown <- !(keys %in% props)
  if (any(unknown)) {
    key <- keys[unknown][1]
    stop(sprintf("unrecognized text filter property: '%s'", key))
  }

  ans <- structure(list(), class = "corpus_text_filter")
  for (prop in props) {
    ans[[prop]] <- filter[[prop]]
  }
  ans
}


as_group <- function(group, n) {
  if (!is.null(group)) {
    group <- as.factor(group)

    if (length(group) != n) {
      stop(paste0(
        "'group' argument has wrong length ('",
        length(group), "'; must be '", n, "'"
      ))
    }
  }

  group
}


as_integer_scalar <- function(name, value, nonnegative = FALSE) {
  if (is.null(value)) {
    return(NULL)
  }
  value <- as_integer_vector(name, value, nonnegative)
  if (length(value) != 1) {
    stop(sprintf("'%s' must have length 1", name))
  }
  value
}


as_integer_vector <- function(name, value, nonnegative = FALSE) {
  if (is.null(value)) {
    return(NULL)
  }

  if (!(is.numeric(value) || all(is.na(value)))) {
    stop(sprintf("'%s' must be integer-valued", name))
  }

  value <- as.integer(value)
  if (nonnegative && any(!is.na(value) & value < 0)) {
    stop(sprintf("'%s' must be non-negative", name))
  }

  value
}


as_kind <- function(kind) {
  if (is.null(kind) || all(is.na(kind))) {
    return(NULL)
  } else if (!is.character(kind)) {
    stop(paste("'kind' must be a character vector"))
  }
  kind <- as_character_vector("kind", kind, utf8 = TRUE)
  kind <- unique(kind[!is.na(kind)])
  kind
}


as_na_print <- function(name, value) {
  if (is.null(value)) {
    return(NULL)
  }
  value <- as_character_scalar(name, value)
  if (is.na(value)) {
    stop(sprintf("'%s' cannot be NA", name))
  }
  value
}


as_names <- function(name, value, n, unique = TRUE) {
  if (is.null(value)) {
    return(NULL)
  }

  value <- as_character_vector(name, value)
  if (!unique && length(value) == 1) {
    value <- rep(value, n)
  }
  if (length(value) != n) {
    stop(sprintf(
      "'%s' has wrong length (%d); must be %d",
      name, length(value), n
    ))
  }
  if (unique && anyDuplicated(value)) {
    stop(sprintf("'%s' contains duplicate values", name))
  }
  value
}


as_ngrams <- function(ngrams) {
  if (is.null(ngrams)) {
    return(NULL)
  }

  if (!is.numeric(ngrams)) {
    stop("'ngrams' must be NULL or an integer vector")
  }

  if (anyNA(ngrams) || !all(is.finite(ngrams) & ngrams >= 1)) {
    stop("'ngrams' entries must be positive integer values")
  }

  if (any(ngrams >= 128)) {
    stop("'ngrams' entries must be below 128")
  }

  if (length(ngrams) == 0) {
    stop("'ngrams' argument cannot have length 0")
  }

  ngrams <- sort(unique(as.integer(ngrams)))
  ngrams
}


as_nonnegative <- function(name, value) {
  if (is.null(value)) {
    return(NULL)
  }
  value <- as_integer_scalar(name, value, nonnegative = TRUE)
  if (is.na(value)) {
    stop(sprintf("'%s' cannot be NA", name))
  }
  value
}


as_option <- function(name, value) {
  if (is.null(value)) {
    return(FALSE)
  }

  if (!(length(value) == 1 && is.logical(value) && !is.na(value))) {
    stop(sprintf("'%s' must be TRUE or FALSE", name))
  }
  as.logical(value)
}


as_rows <- function(name, value) {
  if (is.null(value)) {
    return(NULL)
  }

  value <- as_integer_scalar(name, value)
  if (is.na(value)) {
    stop(sprintf("'%s' cannot be NA", name))
  }

  value
}


as_size <- function(size) {
  if (!(is.numeric(size) && length(size) == 1 && !is.na(size))) {
    stop("'size' must be a finite numeric scalar")
  }

  if (is.nan(size) || !(size >= 1)) {
    stop("'size' must be at least 1")
  }

  size <- floor(size)
  as.double(size)
}


as_snowball_algorithm <- function(name, value) {
  if (is.null(value)) {
    return(value)
  }

  value <- as_character_scalar(name, value)
  isocodes <- c(
    ar = "arabic", da = "danish", de = "german",
    en = "english", es = "spanish", fi = "finnish",
    fr = "french", hu = "hungarian", it = "italian",
    nl = "dutch", no = "norwegian", pt = "portuguese",
    ro = "romanian", ru = "russian", sv = "swedish",
    ta = "tamil", tr = "turkish"
  )
  extra <- "porter"

  stemmers <- c(names(isocodes), sort(c(isocodes, extra)))
  value <- as_enum(name, value, stemmers)
  if (!is.na(i <- match(value, isocodes))) {
    value <- names(isocodes)[[i]]
  }

  value
}


as_stemmer <- function(stemmer) {
  if (is.null(stemmer)) {
    return(NULL)
  }

  if (is.character(stemmer)) {
    stemmer <- as_snowball_algorithm("stemmer", stemmer)
  } else if (!is.function(stemmer)) {
    if (!is.null(attr(stemmer, "class"))) {
      stemmer <- as.function(stemmer)
      if (!is.function(stemmer)) {
        stop("calling 'as.function' on 'stemmer' value did not return a function")
      }
    } else {
      stop("'stemmer' must be a character string, a function, or NULL")
    }
  }
  stemmer
}


as_weights <- function(weights, n) {
  if (!is.null(weights)) {
    weights <- as.numeric(weights)

    if (length(weights) != n) {
      stop(paste0(
        "'weights' argument has wrong length (",
        length(weights), ", must be ", n, ")"
      ))
    }
    if (any(is.nan(weights))) {
      stop("'weights' argument contains a NaN value")
    }
    if (anyNA(weights)) {
      stop("'weights' argument contains a missing value")
    }
    if (any(is.infinite(weights))) {
      stop("'weights' argument contains an infinite value")
    }
  }

  weights
}

as_chars <- as_nonnegative

as_digits <- function(name, value) {
  value <- as_nonnegative(name, value)
  if (!is.null(value) && value > 22) {
    stop(sprintf("'%s' must be less than or equal to 22", name))
  }
  value
}

as_justify <- function(name, value) {
  as_enum(name, value, c("left", "right", "centre", "none"))
}

as_max_print <- as_nonnegative

as_print_gap <- function(name, value) {
  value <- as_nonnegative(name, value)
  if (!is.null(value) && value > 1024) {
    stop(sprintf("'%s' must be less than or equal to 1024", name))
  }
  value
}
