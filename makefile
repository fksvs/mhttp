CC = gcc
CFLAGS = -Wall

mhttp : mhttp.c
	$(CC) -o mhttp mhttp.c

.PHONY : clean
clean :
	rm -f mhttp
