CC = gcc
CFLAGS = -g -I./src
LDFLAGS =
LIBS = .
SRC = src/internal.c src/rvm.c
OBJ = $(SRC:.c=.o)

OUT = lib/liblrvm.a

.c.o:
	$(CC) $(CFLAGS) -c $< -o $@

$(OUT): $(OBJ)
	ar rcs $(OUT) $(OBJ)

test:
	$(CC) $(CFLAGS) tests/abort.c 		-L./lib -llrvm -o bin/abort
	$(CC) $(CFLAGS) tests/basic.c 		-L./lib -llrvm -o bin/basic
	$(CC) $(CFLAGS) tests/multi-abort.c -L./lib -llrvm -o bin/multi-abort
	$(CC) $(CFLAGS) tests/multi.c 		-L./lib -llrvm -o bin/multi
	$(CC) $(CFLAGS) tests/truncate.c 	-L./lib -llrvm -o bin/truncate
	$(CC) $(CFLAGS) tests/remap.c 		-L./lib -llrvm -o bin/remap
	$(CC) $(CFLAGS) tests/multi-trans.c -L./lib -llrvm -o bin/multi-trans
	$(CC) $(CFLAGS) tests/crash.c 		-L./lib -llrvm -o bin/crash


clean :
	@rm -rf src/*.o lib/*.a bin/*
	@echo Cleaned!
