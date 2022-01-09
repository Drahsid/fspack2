BUILD_DIR = build
SRC_DIR = src
C_FILES = $(foreach dir,$(SRC_DIR),$(wildcard $(dir)/*.c))
CPP_FILES = $(foreach dir,$(SRC_DIR),$(wildcard $(dir)/*.cpp))

O_FILES	 = $(foreach file,$(C_FILES),$(BUILD_DIR)/$(file).o) \
           $(foreach file,$(CPP_FILES),$(BUILD_DIR)/$(file).o)

TARGET = fspack

CC = gcc
CPP = g++

default: all

all: dirs $(BUILD_DIR)/$(TARGET)

dirs:
	$(foreach dir,$(SRC_DIR),$(shell mkdir -p $(BUILD_DIR)/$(dir)))

nuke:
	rm -rf $(BUILD_DIR)

clean:
	rm -rf $(BUILD_DIR)

$(BUILD_DIR)/%.c.o: %.c
	$(CC) -c $< -o $@ -Iinclude

$(BUILD_DIR)/%.cpp.o: %.cpp
	$(CPP) -c $< -o $@ -Iinclude

$(BUILD_DIR)/$(TARGET): $(O_FILES)
	$(CC) -o $@ $(O_FILES) -lc
	cp $(BUILD_DIR)/$(TARGET) $(TARGET)

### Settings
.SECONDARY:
.PHONY: all clean default
SHELL = /bin/bash -e -o pipefail