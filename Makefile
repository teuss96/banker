CC = gcc
CFLAGS = -std=c11 -Wall -Wextra -Wpedantic -O2 -g

banker: banker.c
	$(CC) $(CFLAGS) -o $@ banker.c

# exemplo do enunciado, casos limite e casos de erro
test: banker
	sh testes/run.sh ./banker

clean:
	rm -f banker result.txt

.PHONY: test clean
