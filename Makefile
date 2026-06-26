CC     = gcc
CFLAGS = -Wall -Wextra -O2 -g
SRC    = src/mm.c src/utils.c

all: memtest bench

memtest: tests/memtest.c $(SRC)
	$(CC) $(CFLAGS) -o $@ $^

bench: tests/bench.c $(SRC)
	$(CC) $(CFLAGS) -o $@ $^

valgrind: tests/memtest.c $(SRC)
	$(CC) $(CFLAGS) -DN=500 -DMAX_SIZE=256 -o memtest_vg $^
	valgrind --leak-check=full --error-exitcode=1 ./memtest_vg

asan: tests/memtest.c $(SRC)
	$(CC) $(CFLAGS) -fsanitize=address,undefined -o memtest_asan $^
	./memtest_asan

clean:
	rm -f memtest bench memtest_asan memtest_vg
