CC=gcc
CFLAGS=-fsanitize=signed-integer-overflow -fsanitize=undefined -g -std=gnu99 -O2 -Wall -Wextra -Wno-sign-compare -Wno-unused-parameter -Wno-unused-variable -Wshadow -pthread

test1: test1.c sut.o 
	$(CC) -o test1 $(CFLAGS) test1.c sut.o
test2: test2.c sut.o 
	$(CC) -o test2 $(CFLAGS) test2.c sut.o
test3: test3.c sut.o 
	$(CC) -o test3 $(CFLAGS) test3.c sut.o
test4: test4.c sut.o 
	$(CC) -o test4 $(CFLAGS) test4.c sut.o
test5: test5.c sut.o 
	$(CC) -o test5 $(CFLAGS) test5.c sut.o
sut.o: sut.c
	$(CC) -c $(CFLAGS) sut.c
clean:
	rm *.o test1 test2 test3 test4 test5
