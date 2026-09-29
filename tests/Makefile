# tests/Makefile - kemacs test suite
#
# Builds and runs C unit tests for kemacs internals.
# Each test program includes (or links against) the kemacs
# source under test, providing the required global symbols
# via maindef + edef.h and stub implementations for editor
# runtime functions.

CC      = cc
CFLAGS  = -O2 -std=gnu89 -I.. -I../kanji -I../Cstrings
LDFLAGS =

# Prerequisites built by the parent project
CSSO    = ../Css.o
CSTR_A  = ../Cstrings/Cstrings.a
KANJI_A = ../kanji/kanji.a

# eval.o for test_eval (recompiled with -ffunction-sections)
EFLAGS  = -ffunction-sections -fdata-sections
ELDFLAGS = -Wl,--gc-sections

.PHONY: all run clean

# --- test_kanji ---
test_kanji: test_kanji.c test.h
	$(CC) $(CFLAGS) test_kanji.c -o test_kanji

# --- test_search ---
test_search: test_search.c test.h $(CSSO) $(CSTR_A)
	$(CC) $(CFLAGS) test_search.c $(CSSO) $(CSTR_A) -o test_search

# --- test_line ---
test_line: test_line.c test.h $(CSSO) $(CSTR_A)
	$(CC) $(CFLAGS) test_line.c $(CSSO) $(CSTR_A) -o test_line

# --- test_eval ---
# eval.c is recompiled with -ffunction-sections so --gc-sections
# can drop all eval.c code except the tested functions (stoi,
# itoaa, gettyp, stol, ltos).  Only Cxstr (from Css.o) is
# needed for those functions.
eval_gc.o: ../eval.c
	$(CC) $(CFLAGS) $(EFLAGS) -c ../eval.c -o eval_gc.o

test_eval: test_eval.c test.h eval_gc.o $(CSSO)
	$(CC) $(CFLAGS) $(EFLAGS) test_eval.c eval_gc.o $(CSSO) \
		$(ELDFLAGS) -o test_eval

# --- test_batch ---
# Batch-mode test harness: invokes the kemacs binary with -x
# commands.  Requires the kemacs binary to be built first.
test_batch: test_batch.c test.h
	$(CC) $(CFLAGS) test_batch.c -o test_batch

all: test_kanji test_search test_line test_eval test_batch

run: all
	@for t in test_kanji test_search test_line test_eval; do \
		echo "=== Running $$t ==="; \
		./$$t || exit 1; \
	done
	@echo ""
	@echo "=== Running test_batch ==="
	@./test_batch || exit 1
	@echo ""
	@echo "=== All tests passed ==="

clean:
	rm -f test_kanji test_search test_line test_eval test_batch eval_gc.o  test_line_dbg  test_search_dbg
