srcdir = src
compiler = gcc
flags := $(compiler) -Wall -Werror -std=c11

default: main.o mem.o util.o
	$(compiler) main.o mem.o util.o -o nes.out

main.o: main.c cpu.h
	$(compiler) -c main.c

mem.o: mem.h mem.c
	$(compiler) -c mem.c

util.o: util.h util.c
	$(compiler) -c util.c

clean:
	rm -fv *.o *.out
