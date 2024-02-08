CC = gcc
CFLAGS = -Wall -Werror -lcrypto -lssl
DEBUG_FLAGS = -DDEBUG -g

SOURCES = $(wildcard src/*.c)
OBJECTS = $(SOURCES:.c=.o)
TARGET = mhttp

$(TARGET) : $(OBJECTS)
	$(CC) $(CFLAGS) -o $@ $^

debug : CFLAGS += $(DEBUG_FLAGS)
debug : $(TARGET)

.PHONY : clean
clean :
	rm -f $(TARGET) $(OBJECTS)
