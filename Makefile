CC=gcc
CFLAGS ?= -std=c99 -Wall -Wpedantic -g3
LDFLAGS ?= -lm -Wall -Wpedantic -g3
PREFIX?=/usr/bin/

SRC_DIR=src
BUILD_DIR?=build

SOURCES=kit.frontend.c kit.ast.c kit.ast.free.c kit.lex.c kit.var.c kit.list.c kit.list.sort.c kit.map.c builtins/kit.bfunc.c builtins/kit.bfunc.rt.c builtins/kit.bfunc.str.c builtins/kit.bfunc.list.c builtins/kit.bfunc.io.c builtins/kit.bfunc.sys.c builtins/kit.bfunc.math.c builtins/kit.bfunc.rand.c builtins/kit.bfunc.log.c builtins/kit.bfunc.time.c kit.pool.c kit.ldfile.c kit.arena.c kit.cvt.c kit.struct.c kit.cc.c kit.exec.c kit.vm.c

OBJ = $(patsubst %.c,$(BUILD_DIR)/%.o,$(SOURCES))

.PHONY: all clean

all: $(BUILD_DIR) $(BUILD_DIR)/kscript

$(BUILD_DIR)/kscript: $(OBJ)
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
	mkdir -p $(BUILD_DIR)/builtins

$(BUILD_DIR)/%.eb: %.e $(BUILD_DIR)/ec | $(BUILD_DIR)
	$(BUILD_DIR)/ec $< -o $@

clean:
	rm -rf $(BUILD_DIR)

install: $(BUILD_DIR)/kitc $(BUILD_DIR)/kitexec
	ln -sf -T $(realpath $(BUILD_DIR)/kitc) $(PREFIX)/kitc
	ln -sf -T $(realpath $(BUILD_DIR)/kitexec) $(PREFIX)/kitexec
