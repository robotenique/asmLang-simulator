#
# Makefile para Parte 1 do projeto.
#

CFLAGS = -Wall -std=c99 -O2


.PHONY: clean


all: center freq


center: center.o buffer.o error.o
	$(CC) -o $@ $^


freq: freq.o stable.o error.o hash.o buffer.o
	$(CC) -o $@ $^


%.o: %.c include/%.h
	$(CC) $(CFLAGS) -c $< -o $@


%.o: src/%.c
	$(CC) $(CFLAGS) -c $< -o $@


clean:
	rm -f *.o *~ center freq
