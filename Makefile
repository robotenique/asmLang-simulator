#
# Makefile para Parte 1 do projeto.
#

CFLAGS = -Wall -std=c99 -O2


.PHONY: clean


all: parser


parser: parserTest.o buffer.o error.o hash.o optable.o parser.o asmtypes.o stable.o
	$(CC) -o $@ $^


%.o: src/%.c include/%.h
	$(CC) $(CFLAGS) -c $< -o $@


%.o: src/%.c
	$(CC) $(CFLAGS) -c $< -o $@


clean:
	rm -f *.o *~ parser
