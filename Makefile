# =============================================================== #
# Copyright (c) 2026 Raphael Quintao <raphaelquintao@gmail.com>   #
# This file is part of qredshift.                                 #
# SPDX-License-Identifier: Apache-2.0                             #
# =============================================================== #

MAKEFLAGS += --no-print-directory


NAME    := qredshift
VERSION := 1.0.0
REVISION := 1

ARCHS :=  x86_64 i686 aarch64 armv7l powerpc64le riscv64

ARCH ?= $(shell uname -m)

PWD  ?= $(shell pwd)
BASE := $(abspath $(dir $(lastword $(MAKEFILE_LIST))))
BASE := $(if $(filter $(BASE),$(PWD)),.,$(BASE))

TARGET_USER = $(if $(SUDO_USER),$(SUDO_USER),$(USER))
U = $(shell id -u $(TARGET_USER)):$(shell id -g $(TARGET_USER))

SRC_DIR = $(BASE)/src
OBJ_DIR = $(BASE)/build/$(ARCH)
BIN_DIR = $(BASE)/bin/$(ARCH)

# Installation paths
PREFIX         ?= /usr
BINDIR         ?= $(PREFIX)/bin
MANDIR         ?= $(PREFIX)/share/man/man1
BASHCOMPDIR    ?= $(PREFIX)/share/bash-completion/completions


CC ?= cc
CFLAGS += -std=c99 -pedantic-errors -Wall -Wextra -O2 -DVERSION=\"$(VERSION)\" -DNAME=\"$(NAME)\"

LDFLAGS +=
LDLIBS += $(shell pkg-config --libs x11 xrandr xcb xcb-randr) -ldl -lm

TARGET = $(NAME)

PLUGIN_TARGET = libqredshift_wayland_$(VERSION).so


srсs = $(shell find $(SRC_DIR) -name '*.c')

wayland_core = $(filter %backend_wayland.c %qwlr.c %protocol.c,$(srсs))
common_srcs = $(filter %globals.c %qcli.c %x11_utils.c %qramps.c, $(srсs))
wayland_srcs = $(wayland_core) $(common_srcs)

main_srcs    = $(filter-out %backend_wayland.c %qwlr.c %protocol.c,$(srсs))

objs        = $(patsubst $(SRC_DIR)/%.c,$(OBJ_DIR)/%.o,$(main_srcs))
plugin_objs = $(patsubst $(SRC_DIR)/%.c,$(OBJ_DIR)/%.o,$(wayland_srcs))

# Helpers
#qecho = printf "%b\033[%sm%b\033[0m" "$(if $3,$(shell printf "\033[%sm%b\033[0m" $(3) "=> "),)" $(2) $(1)
define qecho
	printf "%b\033[%sm%b\033[0m" $(if $3,$1,"") $(if $3,$3,$2) $(if $3,$2,$1)
endef

.NOTPARALLEL:

.PHONY: all clean-build build build-plugin install uninstall clean .deb .debs help


.DEFAULT: all


all: build build-plugin ## Clean and compile the final binary

clean-build: clean build build-plugin

build: $(TARGET) ## Compile the final binary to /bin
	@$(call qecho, " => ", "Built: ", "0;32")
	@$(call qecho, "bin/$(ARCH)/$(TARGET) \n", "1")

$(OBJ_DIR)/%.o: $(SRC_DIR)/%.c
	@mkdir -p $(dir $@)
	@$(CC) $(CFLAGS) -fPIC -c $< -o $@

$(TARGET): $(objs)
	@mkdir -p $(BIN_DIR)
	@$(call qecho, "Buildind Binary for: ", "1;35")
	@$(call qecho, "$(ARCH) \n", "1")
	@$(call qecho, " => ", "CC: $(CC) \n", "1")
	@$(call qecho, " => ", "Linking Binary to: ", "0;33")
	@$(call qecho, "$@ \n", "1")
	@$(CC) $(LDFLAGS) $(objs) -o $(BIN_DIR)/$@ $(LDLIBS)

build-plugin: $(PLUGIN_TARGET)
	@$(call qecho, " => ", "Built: ", "0;32")
	@$(call qecho, "bin/$(ARCH)/$(PLUGIN_TARGET) \n", "1")

$(PLUGIN_TARGET): $(plugin_objs)
	@mkdir -p $(BIN_DIR)
	@$(call qecho, "Buildind Wayland Plugin for: ", "1;35")
	@$(call qecho, "$(ARCH) \n", "1")
	@$(call qecho, " => ", "Linking Wayland Plugin to: ", "0;33")
	@$(call qecho, "$@ \n", "1")
	@$(CC) -shared -fPIC $(CFLAGS) $(LDFLAGS) $(plugin_objs) -o $(BIN_DIR)/$@ $(shell pkg-config --libs wayland-client)

install: ## Install compiled binary and plugin library (require sudo)
	@install -Dm755 $(BIN_DIR)/$(TARGET) $(DESTDIR)$(BINDIR)/qredshift
	@install -Dm755 $(BIN_DIR)/$(PLUGIN_TARGET) $(DESTDIR)$(PREFIX)/lib/qredshift/$(PLUGIN_TARGET)
	@install -Dm644 data/man/qredshift.1 $(DESTDIR)$(MANDIR)/qredshift.1
	@install -Dm644 data/bash_completion/qredshift $(DESTDIR)$(BASHCOMPDIR)/qredshift

uninstall: ## Uninstall compiled binary (require sudo)
	rm -f $(DESTDIR)$(BINDIR)/qredshift
	rm -rf $(DESTDIR)$(PREFIX)/lib/qredshift
	rm -f $(DESTDIR)$(MANDIR)/qredshift.1
	rm -f $(DESTDIR)$(BASHCOMPDIR)/qredshift

clean: ## Remove build artifacts
	@$(call qecho, "Cleaning... \n", "0;33")
	@rm -rf $(BASE)/build/* $(BASE)/bin/*


.deb: ## Used by docker, dont call it directly
	@chmod +x $(BASE)/scripts/make_deb.sh
	@./$(BASE)/scripts/make_deb.sh $(NAME) $(VERSION) $(ARCH) "$(date)" $(REVISION)

.debs: ## Used by docker, dont call it directly
	@for a in $(ARCHS); do \
		make .deb ARCH=$$a; \
	done

help:
	@grep -E '^[a-zA-Z_-]+:.*?## .*$$' $(MAKEFILE_LIST) | sort | awk 'BEGIN {FS = ":.*?## "}; {printf "\033[36m%-30s\033[0m %s\n", $$1, $$2}'

