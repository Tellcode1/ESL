CC ?= gcc
CFLAGS += -g -Wall -Wpedantic
LDFLAGS += -lm

RUNTIME_CFLAGS=$(CFLAGS) -D_K_RUNTIME_BUILTINS

SCRIPT_CFLAGS=-O

SRC_DIR=.
STD_DIR=../std/src
BUILD_DIR=build

SOURCES=ast.c lex.c var.c
COMPILER_SOURCES=cc.c cfront.c $(SOURCES)
DECOMPILER_SOURCES=dc.c $(SOURCES)
RUNTIME_SOURCES=exec.c $(SOURCES)

COMPILER_OBJ = $(patsubst %.c,$(BUILD_DIR)/%.o,$(COMPILER_SOURCES))
DECOMPILER_OBJ = $(patsubst %.c,$(BUILD_DIR)/%.o,$(DECOMPILER_SOURCES))
RUNTIME_OBJ = $(patsubst %.c,$(BUILD_DIR)/%.o,$(RUNTIME_SOURCES))
SCRIPTS = $(wildcard tests/*.e)
COMPILED_SCRIPTS = $(patsubst tests/%.e,$(BUILD_DIR)/%.eb,$(SCRIPTS))

STD_LIB = $(BUILD_DIR)/libnvstd.a

.PHONY: all clean clean_scripts

all: $(BUILD_DIR) $(BUILD_DIR)/ec $(BUILD_DIR)/dc $(BUILD_DIR)/eexec

$(BUILD_DIR)/ec: $(COMPILER_OBJ) $(STD_LIB)
	$(CC) $(LDFLAGS) $^ -o $@

$(BUILD_DIR)/dc: $(DECOMPILER_OBJ) $(STD_LIB)
	$(CC) $(LDFLAGS) $^ -o $@

$(BUILD_DIR)/eexec: $(RUNTIME_OBJ) $(STD_LIB)
	$(CC) $(LDFLAGS) $^ -o $@

$(BUILD_DIR)/rt/%.o: $(SRC_DIR)/%.c | $(BUILD_DIR)
	@mkdir -p $(dir $@)
	$(CC) $(RUNTIME_CFLAGS) -c $< -o $@

$(BUILD_DIR)/%.o: $(SRC_DIR)/%.c | $(BUILD_DIR)
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/cc/%.o: $(SRC_DIR)/%.c | $(BUILD_DIR)
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR):
	mkdir -p $(BUILD_DIR)

$(STD_LIB): ../std/build/libstd.a
	make -C ../std/ CC='$(CC)' CFLAGS='$(CFLAGS)' LDFLAGS='$(LDFLAGS)' BUILD='$(realpath $(BUILD_DIR))'

$(BUILD_DIR)/%.bin: src/%.dis $(BUILD_DIR)/kcc | $(BUILD_DIR)
	$(BUILD_DIR)/kcc $(SCRIPT_CFLAGS) $< -o $@

clean:
	rm -rf $(BUILD_DIR)