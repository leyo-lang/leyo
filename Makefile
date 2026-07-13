# ------------------------------------------------------------
# Leyo Makefile
# ------------------------------------------------------------

CC = gcc

GIT_COMMIT := $(shell git describe --always --tags)
GIT_DIRTY := $(strip $(shell git diff-index --quiet HEAD -- && echo No || echo Yes))

CFLAGS ?= -Wall -Wextra -pedantic -std=c99
CFLAGS += \
	-DGIT_COMMIT=\"$(GIT_COMMIT)\" \
	-DGIT_DIRTY=\"$(GIT_DIRTY)\"

SRC_DIR   := src
BUILD_DIR := build
BIN_DIR   := bin

include VERSION.mk

ifeq ($(OS),Windows_NT)
TARGET := $(BIN_DIR)/leyo.exe
else
TARGET := $(BIN_DIR)/leyo
endif

# ------------------------------------------------------------
# Sources
# ------------------------------------------------------------

SRCS := $(wildcard $(SRC_DIR)/*.c)
OBJS := $(patsubst $(SRC_DIR)/%.c,$(BUILD_DIR)/%.o,$(SRCS))

GIT_INFO_STAMP := $(BUILD_DIR)/git-info-$(GIT_COMMIT)-$(GIT_DIRTY).stamp

# ------------------------------------------------------------
# Default target
# ------------------------------------------------------------

all: dirs version.h $(TARGET)

# ------------------------------------------------------------
# Directories
# ------------------------------------------------------------

dirs:
ifeq ($(OS),Windows_NT)
	@if not exist "$(BUILD_DIR)" mkdir "$(BUILD_DIR)"
	@if not exist "$(BIN_DIR)" mkdir "$(BIN_DIR)"
else
	mkdir -p $(BUILD_DIR) $(BIN_DIR)
endif

# ------------------------------------------------------------
# Version header
# ------------------------------------------------------------

version.h: VERSION.mk
	@echo #ifndef VERSION_H > include/version.h
	@echo #define VERSION_H >> include/version.h
	@echo #define LEYO_VERSION "$(VERSION)" >> include/version.h
	@echo #endif >> include/version.h

# ------------------------------------------------------------
# Linking
# ------------------------------------------------------------

$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) $^ -o $@ -lm

# ------------------------------------------------------------
# Compilation
# ------------------------------------------------------------

$(BUILD_DIR)/%.o: $(SRC_DIR)/%.c | dirs
	$(CC) $(CFLAGS) -c $< -o $@

# ------------------------------------------------------------
# Git state tracking
# ------------------------------------------------------------

$(BUILD_DIR)/main.o: $(GIT_INFO_STAMP)

$(GIT_INFO_STAMP): | dirs
ifeq ($(OS),Windows_NT)
	-del /Q $(BUILD_DIR)\git-info-*.stamp 2>nul
	@echo $(GIT_COMMIT) $(GIT_DIRTY)>$@
else
	rm -f $(BUILD_DIR)/git-info-*.stamp
	@echo "$(GIT_COMMIT) $(GIT_DIRTY)" > $@
endif

# ------------------------------------------------------------
# Utilities
# ------------------------------------------------------------

run: all
	$(TARGET)

clean:
ifeq ($(OS),Windows_NT)
	-del /Q $(BUILD_DIR)\*.o 2>nul
	-del /Q $(BUILD_DIR)\git-info-*.stamp 2>nul
	-del /Q $(BIN_DIR)\* 2>nul
else
	rm -f $(BUILD_DIR)/*.o
	rm -f $(BUILD_DIR)/git-info-*.stamp
	rm -f $(BIN_DIR)/*
endif

rebuild: clean all

.PHONY: all dirs clean rebuild run version.h