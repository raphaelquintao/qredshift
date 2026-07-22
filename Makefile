# =============================================================== #
# Copyright (c) 2026 Raphael Quintao <raphaelquintao@gmail.com>   #
# SPDX-License-Identifier: Apache-2.0                             #
# =============================================================== #

MAKEFLAGS += --no-print-directory

NAME    := qredshift
VERSION := 0.13.1

ARCHS :=  x86_64 i686 aarch64 armv7l powerpc64le

#x86_64 i686 aarch64 armv7l powerpc64le

DOCKER_IMAGE = qredshift-cross

ARCH ?= $(shell uname -m)
PWD ?= $(shell pwd)
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
CCX ?= g++
CFLAGS += -std=c99 -pedantic-errors -Wall -Wextra -O2 \
-DVERSION=\"$(VERSION)\" -DNAME=\"$(NAME)\"
CXXFLAGS += -Wall -Wextra

LDFLAGS +=
LDLIBS += $(shell pkg-config --libs x11 xrandr xcb xcb-randr) -lm

TARGET = $(NAME)

srсs = $(shell find $(SRC_DIR) -name '*.c')
objs = $(patsubst $(SRC_DIR)/%.c,$(OBJ_DIR)/%.o,$(srсs))

date := $(shell date -R)
id := $(shell grep -E '^ID=' /etc/os-release | cut -d'=' -f2 | tr -d '"')

# Helpers
qecho = printf "\033[%sm%b\033[0m" $(2) $(1)


.PHONY: all build install uninstall clean docker docker-image debug docker-image help

.DEFAULT_GOAL := all



all: clean build ## Clean and build the final binary


build: ## Build the final binary
	@$(call qecho, "Starting build process for ", "1;35")
	@$(call qecho, "$(ARCH) \n", "1")
	@$(call qecho, " => CC : ", "1")
	@$(call qecho, "$(CC) \n", "1")
	@$(call qecho, " => CCX: ", "1")
	@$(call qecho, "$(CCX) \n", "1")
	@$(call qecho, "Compiling... \n", "0;33")
	@make $(BIN_DIR)/$(TARGET) ARCH=$(ARCH)
	@$(call qecho, " => ", "1")
	@$(call qecho, "Built: ", "0;32")
	@$(call qecho, "bin/$(ARCH)/$(TARGET) \n", "1")

$(BIN_DIR)/$(TARGET): $(objs)
	@mkdir -p $(BIN_DIR)
	@#$(call qecho, " => ", "1")
	@$(call qecho, "Linking to: ", "0;33")
	@$(call qecho, "$@ \n", "1")
	@$(CC) $(CFLAGS) $(LDFLAGS) $(objs) -o $@ $(LDLIBS)
	@#$(call qecho, "    Done! \n", "0;32")

$(OBJ_DIR)/%.o: $(SRC_DIR)/%.c
	@mkdir -p $(dir $@)
	@$(call qecho, " => ", "1")
	@$(call qecho, "$< \n", "1")
	@$(CC) $(CFLAGS) -c $< -o $@
	@#$(call qecho, "    Done! \n", "0;32")

install:
	@install -Dm755 $(BIN_DIR)/qredshift $(DESTDIR)$(BINDIR)/qredshift
	@install -Dm644 data/man/qredshift.1 $(DESTDIR)$(MANDIR)/qredshift.1
	@install -Dm644 data/bash_completion/qredshift $(DESTDIR)$(BASHCOMPDIR)/qredshift

uninstall:
	rm -f $(DESTDIR)$(BINDIR)/qredshift
	rm -f $(DESTDIR)$(MANDIR)/qredshift.1
	rm -f $(DESTDIR)$(BASHCOMPDIR)/qredshift

clean: ## Remove build artifacts
	@$(call qecho, "Cleaning... \n", "0;33")
	@rm -rf $(OBJ_DIR) $(BASE)/bin/*



debug: ## Show some debug information about the build environment
	@$(call qecho, "Starting build process for $(NAME)...\n", "1;35")

	@echo "ID: $(id)"
	@echo "Target: $(TARGET)"
	@echo "LDLIBS: $(LDLIBS)"
	@echo "PWD: $(PWD)"
	@echo "BASE: $(BASE)"
	@echo "src_dir: $(SRC_DIR)"
	@echo "srсs: $(srсs)"
	@echo "objs: $(objs)"
	@#echo "Output binaries: $(BINS)"


docker-image: ## Make docker image
	@$(call qecho, "Building Docker image: ", "1;33")
	@$(call qecho, "$(DOCKER_IMAGE)\n", "1")
	@docker build -t "$(DOCKER_IMAGE):latest" $(BASE)/docker;
	@$(call qecho, "=> ", "1")
	@$(call qecho, "Done: ", "0;32")
	@$(call qecho, "$(DOCKER_IMAGE):latest\n", "1")

docker: clean
	@$(call qecho, "Building with DOCKER \n", "1;35")

	@if [ -z "$$(docker images -q $(DOCKER_IMAGE))" ]; then \
		$(call qecho, " => ", "1"); \
		$(call qecho, "Image ", "0;33"); \
		$(call qecho, "$(DOCKER_IMAGE) ", "1"); \
		$(call qecho, "does not exist. Proceeding with build...\n", "0;33"); \
		make docker-image; \
	else \
		$(call qecho, " => ", "1"); \
		$(call qecho, "Image ", "0;34"); \
		$(call qecho, "$(DOCKER_IMAGE) ", "1"); \
		$(call qecho, "already exists.\n", "0;34"); \
	fi


	@docker run -it -v $(BASE):/qredshift -u $(U) $(DOCKER_IMAGE):latest make debs


deb:
	@chmod +x $(BASE)/scripts/make_deb.sh
	@./$(BASE)/scripts/make_deb.sh $(NAME) $(VERSION) $(ARCH) "$(date)"

debs:
	@for a in $(ARCHS); do \
	  make deb ARCH=$$a; \
	done

help:
	@grep -E '^[a-zA-Z_-]+:.*?## .*$$' $(MAKEFILE_LIST) | sort | awk 'BEGIN {FS = ":.*?## "}; {printf "\033[36m%-30s\033[0m %s\n", $$1, $$2}'





