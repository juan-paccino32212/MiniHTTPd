CC = gcc

CFLAGS = -Wall -Wextra -Wpedantic -std=c11 -Iinclude

TARGET = minihttpd

SOURCES = src/main.c src/server.c

OBJECTS = src/main.o src/server.o

all: $(TARGET)

$(TARGET): $(OBJECTS)
	$(CC) $(OBJECTS) -o $(TARGET)

src/%.o: src/%.c
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f $(OBJECTS) $(TARGET)

.PHONY: all clean
