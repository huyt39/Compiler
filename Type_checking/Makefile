CC = gcc
CFLAGS = -Wall -Wextra -std=c99
TARGET = semantics
SOURCES = main.c reader.c parser.c scanner.c semantics.c symtab.c token.c charcode.c error.c debug.c
OBJECTS = $(SOURCES:.c=.o)

all: $(TARGET)

$(TARGET): $(OBJECTS)
	$(CC) $(CFLAGS) -o $(TARGET) $(OBJECTS)

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f $(OBJECTS) $(TARGET)

.PHONY: all clean


