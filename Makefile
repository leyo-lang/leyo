CC = gcc
GIT_COMMIT :=  $(shell git describe --always --tags)
GIT_DIRTY := $(shell git diff-index --quiet HEAD -- && echo No|| echo Yes)

CFLAGS = -Wall -Wextra -pedantic -std=c99 \
         -DGIT_COMMIT=\"$(GIT_COMMIT)\" \
		 -DGIT_DIRTY=\"$(GIT_DIRTY)\"

SRC_DIR = src
BUILD_DIR = build
BIN_DIR = bin
GIT_INFO_STAMP = $(BUILD_DIR)/git-info-$(GIT_COMMIT)-$(GIT_DIRTY).stamp

# find all .c files automatically
SRCS := $(wildcard $(SRC_DIR)/*.c)

# convert them into .o files inside bin/
OBJS := $(patsubst $(SRC_DIR)/%.c,$(BUILD_DIR)/%.o,$(SRCS))

TARGET = $(BIN_DIR)/leyo.exe

# default target
all: version.h $(BUILD_DIR) $(TARGET)

# create build directory
$(BUILD_DIR):
	mkdir $(BUILD_DIR)

$(BIN_DIR):
	mkdir $(BIN_DIR)

include VERSION.mk

version.h: VERSION.mk
	@echo #ifndef VERSION_H > include/version.h
	@echo #define VERSION_H >> include/version.h
	@echo #define LEYO_VERSION "$(VERSION)" >> include/version.h
	@echo #endif >> include/version.h

# link step
$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) $^ -o $@

# compile step (automatic rule for all .c files)
$(BUILD_DIR)/%.o: $(SRC_DIR)/%.c
	$(CC) $(CFLAGS) -c $< -o $@

# keep main.o tied to the current git state so commit/dirty changes rebuild it
$(BUILD_DIR)/main.o: $(GIT_INFO_STAMP)

$(GIT_INFO_STAMP): | $(BUILD_DIR)
ifeq ($(OS),Windows_NT)
	del /Q $(BUILD_DIR)\git-info-*.stamp 2> nul
else
	rm -f $(BUILD_DIR)/git-info-*.stamp
endif
	@echo $(GIT_COMMIT) $(GIT_DIRTY) > $@

# run program
run: all
	$(TARGET)

# clean build files
clean:
ifeq ($(OS),Windows_NT)
	del /Q $(BUILD_DIR)\*.o 2> nul
	del /Q $(BUILD_DIR)\git-info-*.stamp 2> nul
	del /Q $(BIN_DIR)\*.exe 2> nul
else
	rm -f $(BUILD_DIR)/*.o
	rm -f $(BUILD_DIR)/git-info-*.stamp
	rm -f $(BIN_DIR)/*
endif

.PHONY: all clean run
