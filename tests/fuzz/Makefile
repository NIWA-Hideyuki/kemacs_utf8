# Fuzz harness build for kemacs
CC      = cc
CFLAGS  = -std=gnu89 -I../.. -I../../kanji -I../../Cstrings -DKEMACS_TEST \
          -fsanitize=address,undefined -fno-omit-frame-pointer -g
LDFLAGS = -Wl,--gc-sections

EVAL_GC = ../../tests/eval_gc.o
EXEC_GC = exec_gc.o
CSSO    = ../../Css.o

.PHONY: all clean
all: fuzz_stoi fuzz_token

exec_gc.o: ../../exec.c
	$(CC) -O1 -ffunction-sections -fdata-sections $(CFLAGS) -c ../../exec.c -o exec_gc.o

fuzz_stoi: fuzz_stoi.c $(EVAL_GC) $(CSSO)
	$(CC) $(CFLAGS) fuzz_stoi.c $(EVAL_GC) $(CSSO) $(LDFLAGS) -o fuzz_stoi

fuzz_token: fuzz_token.c $(EXEC_GC) $(CSSO)
	$(CC) $(CFLAGS) fuzz_token.c $(EXEC_GC) $(CSSO) $(LDFLAGS) -o fuzz_token

clean:
	rm -f fuzz_stoi fuzz_token exec_gc.o
