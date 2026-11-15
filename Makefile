
# MiniHTTPd - Makefile

# Compilador
CC = gcc

# Directorios
SRC_DIR = src
INC_DIR = include
BUILD_DIR = build

# Ejecutable
TARGET = minihttpd

# Opciones de compilacion
CFLAGS = -Wall -Wextra -Wpedantic -std=c11
CPPFLAGS = -I$(INC_DIR) -MMD -MP
LDFLAGS =

# Archivos fuente
SOURCES = $(wildcard $(SRC_DIR)/*.c)

# Objetos generados
OBJECTS = $(patsubst $(SRC_DIR)/%.c,$(BUILD_DIR)/%.o,$(SOURCES))

# Dependencias automaticas
DEPS = $(OBJECTS:.o=.d)

# Objetivos principales

.PHONY: all clean run debug

all: $(TARGET)

# Enlazado del ejecutable
$(TARGET): $(OBJECTS)
	$(CC) $(OBJECTS) $(LDFLAGS) -o $@

# Compilacion individual de cada modulo
$(BUILD_DIR)/%.o: $(SRC_DIR)/%.c | $(BUILD_DIR)
	$(CC) $(CPPFLAGS) $(CFLAGS) -c $< -o $@

# Crear directorio de compilacion
$(BUILD_DIR):
	mkdir -p $(BUILD_DIR)

# Incluir dependencias generadas por GCC
-include $(DEPS)

# Utilidades

run: $(TARGET)
	./$(TARGET)

debug: CFLAGS += -g -O0
debug: clean all

clean:
	rm -rf $(BUILD_DIR) $(TARGET)
