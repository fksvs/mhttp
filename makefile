CC = gcc
CFLAGS = -Wall -lcrypto -lssl

mhttp : mhttp.c
	$(CC) $(CFLAGS) -o mhttp mhttp.c

.PHONY : clean
clean :
	rm -f mhttp
