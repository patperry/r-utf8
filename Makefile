CC     += -std=c99

CFLAGS += -Wall -Wextra -pedantic -Werror \
	-Wno-cast-qual \
	-Wno-padded \
	-Wno-unused-macros \
	-g

#LDFLAGS +=
LIBS    += -lm
AR      = ar rcu
RANLIB  = ranlib
MKDIR_P = mkdir -p
CURL    = curl

LIB_CFLAGS = \
	-Wno-cast-align \
	-Wno-cast-qual \
	-Wno-float-equal \
	-Wno-missing-prototypes \
	-Wno-sign-conversion \
	-Wno-unreachable-code-break

TEST_CFLAGS = $(shell pkg-config --cflags check) \
	-Wno-double-promotion \
	-Wno-float-equal \
	-Wno-gnu-zero-variadic-macro-arguments \
	-Wno-missing-prototypes \
	-Wno-missing-variable-declarations \
	-Wno-reserved-id-macro

TEST_LIBS = $(shell pkg-config --libs check)

CLDR = https://raw.githubusercontent.com/unicode-cldr/cldr-segments-modern/master
EMOJI = http://www.unicode.org/Public/emoji/5.0
UNICODE = http://www.unicode.org/Public/10.0.0

CORPUS_A = libutf8lite.a
LIB_O	= src/char.o src/encode.o src/error.o src/escape.o src/normalize.o \
	  src/text.o src/textassign.o src/textiter.o

DATA    = data/emoji/emoji-data.txt \
	  data/ucd/CaseFolding.txt \
	  data/ucd/CompositionExclusions.txt \
	  data/ucd/DerivedCoreProperties.txt \
	  data/ucd/EastAsianWidth.txt \
	  data/ucd/PropList.txt \
	  data/ucd/Scripts.txt \
	  data/ucd/UnicodeData.txt \
	  data/ucd/auxiliary/SentenceBreakProperty.txt \
	  data/ucd/auxiliary/WordBreakProperty.txt

TESTS_T = tests/check_charwidth tests/check_text tests/check_unicode
TESTS_O = tests/check_charwidth.o tests/check_text.o tests/check_unicode.o \
		  tests/testutil.o

TESTS_DATA = data/ucd/NormalizationTest.txt

ALL_O = $(LIB_O) $(CORPUS_O) $(STEMMER_O)
ALL_T = $(CORPUS_A) $(CORPUS_T)
ALL_A = $(CORPUS_A)


# Products

all: $(ALL_T)

$(CORPUS_A): $(LIB_O) $(STEMMER_O)
	$(AR) $@ $(LIB_O) $(STEMMER_O)
	$(RANLIB) $@

$(CORPUS_T): $(CORPUS_O) $(CORPUS_A)
	$(CC) -o $@ $(CORPUS_O) $(CORPUS_A) $(LIBS) $(LDFLAGS)


# Data

data/emoji/emoji-data.txt:
	$(MKDIR_P) data/emoji
	$(CURL) -o $@ $(EMOJI)/emoji-data.txt

data/ucd/CaseFolding.txt:
	$(MKDIR_P) data/ucd
	$(CURL) -o $@ $(UNICODE)/ucd/CaseFolding.txt

data/ucd/CompositionExclusions.txt:
	$(MKDIR_P) data/ucd
	$(CURL) -o $@ $(UNICODE)/ucd/CompositionExclusions.txt

data/ucd/DerivedCoreProperties.txt:
	$(MKDIR_P) data/ucd
	$(CURL) -o $@ $(UNICODE)/ucd/DerivedCoreProperties.txt

data/ucd/EastAsianWidth.txt:
	$(MKDIR_P) data/ucd
	$(CURL) -o $@ $(UNICODE)/ucd/EastAsianWidth.txt

data/ucd/NormalizationTest.txt:
	$(MKDIR_P) data/ucd
	$(CURL) -o $@ $(UNICODE)/ucd/NormalizationTest.txt

data/ucd/UnicodeData.txt:
	$(MKDIR_P) data/ucd
	$(CURL) -o $@ $(UNICODE)/ucd/UnicodeData.txt


# Generated Sources

src/private/casefold.h: util/gen-casefold.py \
		data/ucd/CaseFolding.txt
	$(MKDIR_P) src/private
	./util/gen-casefold.py > $@

src/private/charwidth.h: util/gen-charwidth.py util/property.py util/unicode_data.py \
		data/emoji/emoji-data.txt data/ucd/DerivedCoreProperties.txt \
		data/ucd/EastAsianWidth.txt data/ucd/UnicodeData.txt
	$(MKDIR_P) src/private
	./util/gen-charwidth.py > $@

src/private/combining.h: util/gen-combining.py util/unicode_data.py \
		data/ucd/UnicodeData.txt
	$(MKDIR_P) src/private
	./util/gen-combining.py > $@

src/private/compose.h: util/gen-compose.py util/unicode_data.py \
		data/ucd/CompositionExclusions.txt data/ucd/UnicodeData.txt
	$(MKDIR_P) src/private
	./util/gen-compose.py > $@

src/private/decompose.h: util/gen-decompose.py util/unicode_data.py \
		data/ucd/UnicodeData.txt
	$(MKDIR_P) src/private
	./util/gen-decompose.py > $@

src/private/normalization.h: util/gen-normalization.py \
		data/ucd/DerivedNormalizationProps.txt
	$(MKDIR_P) src/private
	./util/gen-normalization.py > $@


# Tests

tests/check_charwidth: tests/check_charwidth.o tests/testutil.o $(CORPUS_A)
	$(CC) -o $@ $^ $(LIBS) $(TEST_LIBS) $(LDFLAGS)

tests/check_text: tests/check_text.o tests/testutil.o $(CORPUS_A)
	$(CC) -o $@ $^ $(LIBS) $(TEST_LIBS) $(LDFLAGS)

tests/check_unicode: tests/check_unicode.o $(CORPUS_A) \
		data/ucd/NormalizationTest.txt
	$(CC) -o $@ tests/check_unicode.o $(CORPUS_A) \
		$(LIBS) $(TEST_LIBS) $(LDFLAGS)


# Special Rules

check: $(TESTS_T) $(TESTS_T:=.test)

clean:
	$(RM) -r $(ALL_O) $(ALL_T) $(TESTS_O) $(TESTS_T)

data: $(DATA) $(TESTS_DATA)

doc:
	doxygen

%.test: %
	$<

lib/%.o: lib/%.c
	$(CC) -c $(CFLAGS) $(LIB_CFLAGS) $(CPPFLAGS) $< -o $@

tests/%.o: tests/%.c
	$(CC) -c $(CFLAGS) $(TEST_CFLAGS) $(CPPFLAGS) $< -o $@


.PHONY: all check clean data doc

src/char.o: src/char.c src/private/charwidth.h src/utf8lite.h

src/encode.o: src/encode.c src/utf8lite.h

src/error.o: src/error.c src/utf8lite.h

src/escape.o: src/escape.c src/utf8lite.h

src/normalize.o: src/normalize.c src/private/casefold.h \
	src/private/combining.h src/private/compose.h src/private/decompose.h \
	src/utf8lite.h

src/text.o: src/text.c src/utf8lite.h

src/textassign.o: src/textassign.c src/utf8lite.h

src/textiter.o: src/textiter.c src/utf8lite.h

tests/check_charwidth.o: tests/check_charwidth.c src/utf8lite.h \
	tests/testutil.h
tests/check_text.o: tests/check_text.c src/utf8lite.h tests/testutil.h
tests/check_unicode.o: tests/check_unicode.c src/utf8lite.h tests/testutil.h
tests/testutil.o: tests/testutil.c src/utf8lite.h tests/testutil.h
