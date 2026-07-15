# SPDX-License-Identifier: CC0-1.0
#
# SPDX-FileContributor: Antonio Niño Díaz, 2023

BLOCKSDS	?= /opt/blocksds/core
BLOCKSDSEXT	?= /opt/blocksds/external

export TARGET := cart_flasher
CART_FLASHER_COMMIT ?= $(shell git rev-parse --short HEAD 2>/dev/null || echo unknown)
export CART_FLASHER_COMMIT
CART_FLASHER_VERSION ?= $(shell git describe --tags --abbrev=0 2>/dev/null || echo "v0.1-alpha")
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
