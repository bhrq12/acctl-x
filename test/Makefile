# Makefile for AC Controller tests

CC = gcc
CFLAGS = -Wall -Wextra -O2 -I../src/include
LDFLAGS = -lpthread -lm

TESTS = test_aphash test_sec

all: $(TESTS)

test_aphash: test_aphash.c ../src/ac/aphash.c
	$(CC) $(CFLAGS) -o $@ $^

test_sec: test_sec.c ../src/lib/sec.c ../src/lib/sha256.c
	$(CC) $(CFLAGS) -o $@ $^

clean:
	rm -f $(TESTS)

run:
	./test_aphash
	./test_sec
