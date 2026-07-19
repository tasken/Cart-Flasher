# SPDX-License-Identifier: CC0-1.0
#
# SPDX-FileContributor: Antonio Niño Díaz, 2023

BLOCKSDS	?= /opt/blocksds/core
BLOCKSDSEXT	?= /opt/blocksds/external

export TARGET := cart_flasher
# Every local build is a dev build, full stop -- no need to distinguish a
# clean checkout from a dirty one here (git diff --quiet only sees tracked
# files anyway, so it never covered untracked adds). This -dev marker is
# static and applies uniformly wherever CART_FLASHER_COMMIT is used: filename,
# banner, splash. CI (nightly/release) overrides CART_FLASHER_COMMIT on the
# command line, so this default -- and its -dev suffix -- never reaches them.
# This repo's tags are lightweight, so plain describe (no --tags) skips them
# and always falls back to the bare hash -- keep it that way, or an annotated
# tag would turn this into a tag-relative string.
CART_FLASHER_COMMIT ?= $(shell git describe --always --abbrev=7 2>/dev/null || echo unknown)-dev
export CART_FLASHER_COMMIT
# No tag is reachable from HEAD in a shallow clone (actions/checkout's default
# without fetch-depth: 0) or a source archive with no .git at all -- "unknown"
# admits that, instead of stamping every such build with the same stale hardcoded
# release tag regardless of how far the tree has actually moved on.
CART_FLASHER_VERSION ?= $(shell git describe --tags --abbrev=0 2>/dev/null || echo unknown)
export CART_FLASHER_VERSION
CART_FLASHER_BRANCH ?= $(shell git rev-parse --abbrev-ref HEAD 2>/dev/null || echo unknown)
# Branch names may contain '/', which would break the output path.
CART_FLASHER_BRANCH := $(subst /,-,$(CART_FLASHER_BRANCH))
ifeq ($(CART_FLASHER_BRANCH),main)
export NDS_OUT := $(TARGET)-$(CART_FLASHER_COMMIT).nds
else
export NDS_OUT := $(TARGET)-$(CART_FLASHER_BRANCH)-$(CART_FLASHER_COMMIT).nds
endif
export TOPDIR := $(CURDIR)

GAME_TITLE     := Cart-Flasher
GAME_AUTHOR    := @tasken
GAME_ICON      := resources/icon.png

# Banner version line: release builds pass BANNER_VERSION=<tag> explicitly;
# everything else (nightlies, local builds) shows the commit.
BANNER_VERSION ?= Commit: $(CART_FLASHER_COMMIT)
GAME_FULL_TITLE := $(GAME_TITLE);Developed by $(GAME_AUTHOR);$(BANNER_VERSION)

# Source code paths
ARM9DIR		:= arm9
ARM7DIR		:= arm7

# Tools
MAKE		:= make
RM		:= rm -rf

ifeq ($(VERBOSE),1)
V		:=
else
V		:= @
endif

.PHONY: all clean arm9 arm7

all: $(NDS_OUT)

clean:
	@echo "  CLEAN"
	$(V)$(MAKE) -C $(ARM9DIR) clean --no-print-directory
	$(V)$(MAKE) -C $(ARM7DIR) clean --no-print-directory
	$(V)$(RM) $(TARGET)-*.nds

arm9:
	$(V)+$(MAKE) -C $(ARM9DIR) --no-print-directory

arm7:
	$(V)+$(MAKE) -C $(ARM7DIR) --no-print-directory

$(NDS_OUT) : arm9 arm7
	@echo "  NDSTOOL $@"
	$(V)$(BLOCKSDS)/tools/ndstool/ndstool -c $@ \
		-7 $(ARM7DIR)/$(TARGET).elf -9 $(ARM9DIR)/$(TARGET).elf \
		-b $(GAME_ICON) "$(GAME_FULL_TITLE)"
