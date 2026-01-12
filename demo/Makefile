BUILD_TYPE ?= debug

CC = gcc
CFLAGS = -w -std=gnu11
LDFLAGS = 

SRC_DIR = .
BUILD_DIR = build
BIN_DIR = $(BUILD_DIR)/bin
OBJ_DIR = $(BUILD_DIR)/obj
EXE_NAME = filetool

TARGET = $(BIN_DIR)/$(EXE_NAME)

SRCS = $(wildcard $(SRC_DIR)/*.c) \
					../src/cli.c
OBJS = $(SRCS:$(SRC_DIR)/%.c=$(OBJ_DIR)/%.o)

ifeq ($(BUILD_TYPE),debug)
	export CFLAGS += -g -O0 -DDEBUG
else
	export CFLAGS += -O2 -DNDEBUG
endif

all: $(TARGET)

$(TARGET): $(OBJS) | $(BIN_DIR)
	$(CC) $(OBJS) -o $@ $(LDFLAGS)

$(OBJ_DIR)/%.o: $(SRC_DIR)/%.c | $(OBJ_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(BIN_DIR) $(OBJ_DIR):
	mkdir -p $@

run: $(TARGET)
	./$(TARGET)

clean:
	rm -rf $(BUILD_DIR) 

.PHONY: all clean
