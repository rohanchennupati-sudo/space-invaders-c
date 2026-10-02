CC     = gcc
CFLAGS = -Wall -Wextra -O2

game: src/main.c
	$(CC) $(CFLAGS) -o game src/main.c

clean:
	rm -f game

.PHONY: clean
