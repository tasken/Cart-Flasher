ifeq ($(strip $(BLOCKSDS)),)
$(error "Please set BLOCKSDS in your environment")
endif
WONDERFUL_TOOLCHAIN	?= /opt/wonderful
ARM_NONE_EABI_PATH	?= $(WONDERFUL_TOOLCHAIN)/toolchain/gcc-arm-none-eabi/bin/
PREFIX		?= $(ARM_NONE_EABI_PATH)arm-none-eabi-
CC		:= $(PREFIX)gcc
CXX		:= $(PREFIX)g++
AR		:= $(PREFIX)ar

ARCH		:=	-marm -mthumb-interwork -masm-syntax-unified
ARCHFLAGS	:=	-march=armv5te -mtune=arm946e-s \
				-fomit-frame-pointer -ffunction-sections -fdata-sections -ffast-math \
				-DARM9 -DNCGC_PLATFORM_NTR

CFLAGS		:=	$(CFLAGS) $(ARCH) $(ARCHFLAGS)
CXXFLAGS	:=	$(CXXFLAGS) $(ARCH) $(ARCHFLAGS)
ASFLAGS		:=	$(ASFLAGS) -g $(ARCH)

CFILES		:=	$(CFILES) platform_ntr.c