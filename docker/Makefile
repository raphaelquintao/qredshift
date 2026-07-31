# =============================================================== #
# Copyright (c) 2026 Raphael Quintao <raphaelquintao@gmail.com>   #
# This file is part of qredshift.                                 #
# SPDX-License-Identifier: Apache-2.0                             #
# =============================================================== #

IMAGE_NAME := qredshift-cross
IMAGE_VER  := debian-13
IMAGE_TAG  := $(IMAGE_NAME):$(IMAGE_VER)

GH_USER := raphaelquintao
REMOTE_TAG := ghcr.io/$(GH_USER)/$(IMAGE_TAG)

ARCH ?= $(shell uname -m)
PWD  := $(shell pwd)
BASE := $(abspath $(dir $(lastword $(MAKEFILE_LIST))))
BASE := $(if $(filter $(BASE),$(PWD)),.,$(BASE))


qecho = printf "%b\033[%sm%b\033[0m" $(if $3,$1,"") $(if $3,$3,$2) $(if $3,$2,$1)


.NOTPARALLEL:

.PHONY: debug publish-base-image docker help

debug:
	@$(call qecho, "Debug\n", "1;35")
#	@$(call qecho, " => ", "$(IMAGE_BASE) \n", "1")
#	@$(call qecho, " => ", "$(IMAGE_FULL) \n", "1")
	@$(call qecho, " => ", "$(IMAGE_TAG) \n", "1")
	@$(call qecho, " => ", "$(PWD) \n", "1")
	@$(call qecho, " => ", "$(BASE) \n", "1")

publish-base-image:
	@$(call qecho, "Building base Debian: ", "1;33")
	@$(call qecho, "$(IMAGE_TAG)\n", "1")
	@docker build --network=host -t $(REMOTE_TAG) -f Dockerfile  .
	@$(call qecho, " => ", "Publishing image to GitHub Container Registry... \n", "35")
	docker push $(REMOTE_TAG)
	@$(call qecho, " => ", "Done!\n", "0;32")


docker:
	@$(call qecho, "Building with Docker image: ", "1;33")
	@$(call qecho, "$(REMOTE_TAG)\n", "1")
	@docker run --rm -it \
	-e "ARCH=$(ARCH)" \
	-e GPG_PRIVATE_KEY \
	-v "$(BASE)/..":/home/q/qredshift \
	-v /tmp:/tmp \
	-u q \
	$(REMOTE_TAG) \
	bash -c "cd qredshift && make .debs";



help:
	@grep -E '^[a-zA-Z_-]+:.*?## .*$$' $(MAKEFILE_LIST) | sort | awk 'BEGIN {FS = ":.*?## "}; {printf "\033[36m%-30s\033[0m %s\n", $$1, $$2}'
