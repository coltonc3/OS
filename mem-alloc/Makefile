CC=gcc
CFLAGS=-fsanitize=signed-integer-overflow -w -Wint-conversion -Wextra -fsanitize=undefined -g -std=gnu99 -O2 -Wall -Wextra -Wno-sign-compare -Wno-unused-parameter -Wno-unused-variable -Wshadow -pthread

test: a3_test.c sma.o
	$(CC) -o test $(CFLAGS)  a3_test.c sma.o
sma.o: sma.c
	$(CC) -c $(CFLAGS) sma.c
