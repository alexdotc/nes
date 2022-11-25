binout = nes.out
compiler = gcc
flags := $(compiler) -Wall -Werror -std=c11

default: main.o cpu.o mem.o nes.o util.o
	$(flags) main.o cpu.o mem.o nes.o util.o -o $(binout)

main.o: main.c
	$(flags) -c main.c

cpu.o: cpu.h cpu.c
	$(flags) -c cpu.c

mem.o: mem.h mem.c
	$(flags) -c mem.c

nes.o: nes.h nes.c
	$(flags) -c nes.c

util.o: util.h util.c
	$(flags) -c util.c

clean:
	rm -fv *.o *.out

memcheck: default
	valgrind --tool=memcheck --leak-check=full ./$(binout)
