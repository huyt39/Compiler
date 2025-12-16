# Makefile for KPL Parser
# @copyright (c) 2008, Hedspi, Hanoi University of Technology

CC = gcc
CFLAGS = -Wall -g -std=c99
TARGET = symtab
SOURCES = main.c parser.c scanner.c reader.c token.c symtab.c error.c debug.c charcode.c
OBJECTS = $(SOURCES:.c=.o)

all: $(TARGET)

$(TARGET): $(OBJECTS)
	$(CC) $(CFLAGS) -o $(TARGET) $(OBJECTS)

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f $(OBJECTS) $(TARGET)

test: $(TARGET)
	@echo "Testing with example1.kpl:"
	./$(TARGET) tests/example1.kpl
	@echo "\nTesting with example3.kpl:"
	./$(TARGET) example3.kpl

.PHONY: all clean test

