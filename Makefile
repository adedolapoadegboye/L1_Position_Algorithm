# Makefile for GPS L1 Position Resolver

# Set this to 'debug' or 'release'
BUILD := debug

# Directories
SRC_DIR := src
OBJ_DIR := build
INC_DIR := include
BIN_DIR := bin

# Files
TARGET := $(BIN_DIR)/gps_resolver
SRCS := $(wildcard $(SRC_DIR)/*.c)
OBJS := $(patsubst $(SRC_DIR)/%.c, $(OBJ_DIR)/%.o, $(SRCS))

 Compiler
CC := gcc
CFLAGS_COMMON := -Wall -Wextra -Werror -pedantic -std=c11 -Wshadow -Wconversion -Wunused-parameter -I$(INC_DIR)

ifeq ($(BUILD),debug)
    CFLAGS := $(CFLAGS_COMMON) -g -O0
else ifeq ($(BUILD),release)
    CFLAGS := $(CFLAGS_COMMON) -O2
else
    $(error Unknown build type: $(BUILD))
endif

# Create target
$(TARGET): $(OBJS) | $(BIN_DIR)
	$(CC) $(OBJS) -o $@

# Compile source files to object files
$(OBJ_DIR)/%.o: $(SRC_DIR)/%.c | $(OBJ_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

# Create necessary directories
$(OBJ_DIR):
	mkdir -p $(OBJ_DIR)

$(BIN_DIR):
	mkdir -p $(BIN_DIR)

# Clean up build files
.PHONY: clean
clean:
	rm -rf $(OBJ_DIR) $(BIN_DIR)

# Clean only object files (preserve binary)
.PHONY: clean-obj
clean-obj:
	rm -rf $(OBJ_DIR)

# Clean only the built executable
.PHONY: clean-bin
clean-bin:
	rm -rf $(BIN_DIR)

# Build the full project
.PHONY: all
all: $(TARGET)

# Rebuild everything from scratch
.PHONY: rebuild
rebuild: clean all

# Run the program (for quick testing)
.PHONY: run
run: $(TARGET)
	./$(TARGET)

# Generate Doxygen documentation
.PHONY: docs
docs:
	doxygen Doxyfile

# Show build type and flags
.PHONY: info
info:
	@echo "Build type: $(BUILD)"
	@echo "CFLAGS: $(CFLAGS)"
