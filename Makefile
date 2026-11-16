CC = gcc
CFLAGS = -std=c11 -Wall -Wextra -Wpedantic -O2 -g

banker: banker.c
	$(CC) $(CFLAGS) -o $@ banker.c

clean:
	rm -f banker result.txt

.PHONY: clean
