# SPDX-License-Identifier: CC0-1.0
#
# SPDX-FileContributor: Antonio Niño Díaz, 2023

BLOCKSDS	?= /opt/blocksds/core
BLOCKSDSEXT	?= /opt/blocksds/external

export TARGET := cart_flasher
# Local builds are always "dev" builds -- no clean/dirty distinction. Prefix,
# not suffix, to match nightly's cart_flasher-nightly-<hash>.nds. CI overrides
# this on the command line.
CART_FLASHER_COMMIT ?= dev-$(shell git describe --always --abbrev=7 2>/dev/null || echo unknown)
export CART_FLASHER_COMMIT
# Falls back to "unknown" (not a stale hardcoded version) when no tag is
# reachable, e.g. a shallow clone or a source archive with no .git.
CART_FLASHER_VERSION ?= $(shell git describe --tags --abbrev=0 2>/dev/null || echo unknown)
export CART_FLASHER_VERSION
# Branch names can legally contain shell metacharacters (" $ ( ) ` ;) that
# Make would splice unescaped into the ndstool recipe below. Filtered to a
# safe whitelist in this same shell call -- filtering again in a later
# $(shell ...) would just re-paste the unsafe text into a new shell command.
CART_FLASHER_BRANCH ?= $(shell BRANCH=$$(git rev-parse --abbrev-ref HEAD 2>/dev/null); if [ -z "$$BRANCH" ]; then echo unknown; else printf '%s' "$$BRANCH" | tr -cd 'A-Za-z0-9._/-'; fi)
# '/' would break the output path.
CART_FLASHER_BRANCH := $(subst /,-,$(CART_FLASHER_BRANCH))
ifeq ($(CART_FLASHER_BRANCH),main)
export NDS_OUT := $(TARGET)-$(CART_FLASHER_COMMIT).nds
else
export NDS_OUT := $(TARGET)-$(CART_FLASHER_BRANCH)-$(CART_FLASHER_COMMIT).nds
endif
export TOPDIR := $(CURDIR)

# Build provenance shown on the splash. "Dev" to match CART_FLASHER_COMMIT's
# dev- prefix above. CI overrides to Nightly/Release.
CART_FLASHER_BUILD_KIND ?= Dev
export CART_FLASHER_BUILD_KIND

GAME_TITLE     := Cart-Flasher
GAME_AUTHOR    := @tasken
GAME_ICON      := resources/icon.png

# Banner version line, labeled with build provenance. CI passes
# BANNER_VERSION explicitly; this default is for local builds only. Branch is
# folded in on non-main so a feature-branch build doesn't look like main's.
ifeq ($(CART_FLASHER_BRANCH),main)
BANNER_VERSION ?= Dev: $(CART_FLASHER_COMMIT)
else
BANNER_VERSION ?= Dev ($(CART_FLASHER_BRANCH)): $(CART_FLASHER_COMMIT)
endif
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
