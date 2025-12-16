CC = gcc
CFLAGS = -Wall -g
TARGET = scanner
SOURCES = scanner.c token.c reader.c charcode.c error.c
OBJECTS = $(SOURCES:.c=.o)

all: $(TARGET)

$(TARGET): $(OBJECTS)
	$(CC) $(CFLAGS) -o $(TARGET) $(OBJECTS)

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f $(OBJECTS) $(TARGET)

test: $(TARGET)
	./$(TARGET) test/example1.kpl

.PHONY: all clean test

