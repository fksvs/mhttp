CC = gcc
CFLAGS = -Wall -Wextra -Werror -Wpedantic
TLS_FLAGS = -lcrypto -lssl
DEBUG_FLAGS = -DDEBUG -g

SOURCES = $(wildcard src/*.c)
OBJECTS = $(SOURCES:.c=.o)
TARGET = mhttp

$(TARGET) : $(OBJECTS)
	$(CC) $(CFLAGS) -o $@ $^ $(TLS_FLAGS)

debug : CFLAGS += $(DEBUG_FLAGS)
debug : $(TARGET)

.PHONY : clean
clean :
	rm -f $(TARGET) $(OBJECTS)
