CC = gcc
CFLAGS = -Wall -lcrypto -lssl

SOURCES = $(wildcard src/*.c)
OBJECTS = $(SOURCES:.c=.o)
TARGET = mhttp

$(TARGET) : $(OBJECTS)
	$(CC) $(CFLAGS) -o $@ $^

.PHONY : clean
clean :
	rm -f $(TARGET) $(OBJECTS)
