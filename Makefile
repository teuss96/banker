CC = gcc
CFLAGS = -std=c11 -Wall -Wextra -Wpedantic -O2 -g

banker: banker.c
	$(CC) $(CFLAGS) -o $@ banker.c

# exemplo do enunciado, casos limite e casos de erro
test: banker
	sh testes/run.sh ./banker

# mesma suite com AddressSanitizer e UndefinedBehaviorSanitizer
asan: banker.c
	$(CC) $(CFLAGS) -fsanitize=address,undefined -fno-sanitize-recover=all -o banker_asan banker.c
	sh testes/run.sh ./banker_asan

clean:
	rm -f banker banker_asan result.txt

.PHONY: test asan clean
