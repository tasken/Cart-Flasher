#---------------------------------------------------------------------------------
.SUFFIXES:
#---------------------------------------------------------------------------------
ifeq ($(strip $(DEVKITARM)),)
$(error "Please set DEVKITARM in your environment. export DEVKITARM=<path to>devkitARM")
endif

ifneq (,$(shell which python3))
PYTHON	:= python3
else ifneq (,$(shell which python2))
PYTHON	:= python2
else ifneq (,$(shell which python))
PYTHON	:= python
else
$(error "Python not found in PATH, please install it.")
endif

export TARGET := cart_flasher
CART_FLASHER_COMMIT ?= $(shell git rev-parse --short HEAD 2>/dev/null || echo unknown)
export CART_FLASHER_COMMIT
export NDS_OUT := $(TARGET)-$(CART_FLASHER_COMMIT).nds
export TOPDIR := $(CURDIR)

# specify a directory which contains the nitro filesystem
# this is relative to the Makefile
NITRO_FILES :=

# These set the information text in the nds file
GAME_TITLE     := Cart-Flasher
#GAME_SUBTITLE  := built with devkitARM
GAME_AUTHOR    := @tasken

GAME_BANNER := banner.bin

ifeq ($(strip $(GAME_SUBTITLE)),)
	export GAME_FULL_TITLE := $(GAME_TITLE);$(GAME_AUTHOR)
else
	export GAME_FULL_TITLE := $(GAME_TITLE);$(GAME_SUBTITLE);$(GAME_AUTHOR)
endif

include $(DEVKITARM)/ds_rules

.PHONY: checkarm7 checkarm9 clean

#---------------------------------------------------------------------------------
# main targets
#---------------------------------------------------------------------------------
all: checkarm7 checkarm9 $(NDS_OUT)

#---------------------------------------------------------------------------------
checkarm7:
	$(MAKE) -C arm7

#---------------------------------------------------------------------------------
checkarm9:
	$(MAKE) -C arm9

#---------------------------------------------------------------------------------
$(NDS_OUT) : $(NITRO_FILES) arm7/$(TARGET).elf arm9/$(TARGET).elf
	ndstool	-c $(NDS_OUT) -7 arm7/$(TARGET).elf -9 arm9/$(TARGET).elf \
		-t banner.bin \
		$(_ADDFILES)

#---------------------------------------------------------------------------------
arm7/$(TARGET).elf:
	$(MAKE) -C arm7

#---------------------------------------------------------------------------------
arm9/$(TARGET).elf:
	$(MAKE) -C arm9

#---------------------------------------------------------------------------------
clean:
	$(MAKE) -C arm9 clean
	$(MAKE) -C arm7 clean
	rm -f $(TARGET)-*.nds
