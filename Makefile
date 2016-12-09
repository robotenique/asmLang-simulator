#
# Makefile para Parte 3 do projeto.
#

CFLAGS = -Wall --std=c99 -O2


.PHONY: clean


all: macas


parser: macas.o asm.o buffer.o error.o optable.o parser.o asmtypes.o stable.o defaultops.o
	$(CC) -o $@ $^


%.o: src/%.c include/%.h
	$(CC) $(CFLAGS) -c $< -o $@


%.o: src/%.c
	$(CC) $(CFLAGS) -c $< -o $@


clean:
	rm -f *.o *~ *.out macas
