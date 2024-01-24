#
# Makefile for the shell lab
#

CFLAGS = -ggdb3 -Wall -pedantic -g -fstack-protector-all -fsanitize=address
sh3650: sh3650.c parser.c
	gcc sh3650.c parser.c -o sh3650 $(CFLAGS)

clean:
	rm *.o sh3650
