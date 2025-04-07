# Compiler
CC = gcc

# Source and build directories
SRC_DIR = src
INC_DIR = include
BIN_DIR = bin
DEBUG_DIR = $(BIN_DIR)/debug
RELEASE_DIR = $(BIN_DIR)/release

# Platform-specific executable extension
ifeq ($(OS),Windows_NT)
    EXE := .exe
    RM := del /Q
    MKDIR := mkdir
else
    EXE :=
    RM := rm -f
    MKDIR := mkdir -p
endif

# Executable names
TARGET = FourSight
DEBUG_TARGET = $(DEBUG_DIR)/$(TARGET)$(EXE)
RELEASE_TARGET = $(RELEASE_DIR)/$(TARGET)$(EXE)

# Compiler flags
CFLAGS = -Wall -Wextra -std=c11 -I$(INC_DIR)
DEBUG_FLAGS = -fsanitize=address -g -O0
RELEASE_FLAGS = -O3 -march=native -mavx2 -mfma

# Source and object files
SRCS = $(wildcard $(SRC_DIR)/*.c)
DEBUG_OBJS = $(patsubst $(SRC_DIR)/%.c, $(DEBUG_DIR)/%.o, $(SRCS))
RELEASE_OBJS = $(patsubst $(SRC_DIR)/%.c, $(RELEASE_DIR)/%.o, $(SRCS))

# Default target
all: release

# Release build
release: $(RELEASE_TARGET)

$(RELEASE_TARGET): $(RELEASE_OBJS) | $(RELEASE_DIR)
	$(CC) $(CFLAGS) $(RELEASE_FLAGS) $^ -o $@

$(RELEASE_DIR)/%.o: $(SRC_DIR)/%.c | $(RELEASE_DIR)
	$(CC) $(CFLAGS) $(RELEASE_FLAGS) -c $< -o $@

# Debug build
debug: $(DEBUG_TARGET)

$(DEBUG_TARGET): $(DEBUG_OBJS) | $(DEBUG_DIR)
	$(CC) $(CFLAGS) $(DEBUG_FLAGS) $^ -o $@

$(DEBUG_DIR)/%.o: $(SRC_DIR)/%.c | $(DEBUG_DIR)
	$(CC) $(CFLAGS) $(DEBUG_FLAGS) -c $< -o $@

# Create bin subdirectories
$(DEBUG_DIR) $(RELEASE_DIR):
	$(MKDIR) $@

# Clean build artifacts
clean:
	-$(RM) $(DEBUG_DIR)/*.o $(DEBUG_TARGET)
	-$(RM) $(RELEASE_DIR)/*.o $(RELEASE_TARGET)

.PHONY: all release debug clean