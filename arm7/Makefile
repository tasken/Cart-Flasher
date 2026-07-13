# SPDX-License-Identifier: CC0-1.0
#
# SPDX-FileContributor: Antonio Niño Díaz, 2023

export BLOCKSDS			?= /opt/blocksds/core

# Source code paths
SOURCEDIRS	:= source
INCLUDEDIRS	:= include

DEFINES		:= -DARM7

LIBS		:= -lnds7
LIBDIRS		:= $(BLOCKSDS)/libs/libnds

NAME		:= arm7
BUILDDIR	:= build
ELF		:= $(CURDIR)/$(TARGET).elf
MAP		:= $(CURDIR)/$(TARGET).map

WONDERFUL_TOOLCHAIN	?= /opt/wonderful
ARM_NONE_EABI_PATH	?= $(WONDERFUL_TOOLCHAIN)/toolchain/gcc-arm-none-eabi/bin/
PREFIX		:= $(ARM_NONE_EABI_PATH)arm-none-eabi-
CC		:= $(PREFIX)gcc
LD		:= $(PREFIX)gcc
OBJDUMP		:= $(PREFIX)objdump
RM		:= rm -rf
MKDIR		:= mkdir -p

ifeq ($(VERBOSE),1)
V		:=
else
V		:= @
endif

SOURCES_S	:= $(shell find -L $(SOURCEDIRS) -name "*.s")
SOURCES_C	:= $(shell find -L $(SOURCEDIRS) -name "*.c")

ARCH		:= -marm -mthumb-interwork -mcpu=arm7tdmi
SPECS		:= $(BLOCKSDS)/sys/crts/ds_arm7.specs

INCLUDEFLAGS	:= $(foreach path,$(INCLUDEDIRS),-I$(path)) \
		   $(foreach path,$(LIBDIRS),-I$(path)/include)

LIBDIRSFLAGS	:= $(foreach path,$(LIBDIRS),-L$(path)/lib)

ASFLAGS		+= -x assembler-with-cpp $(DEFINES) $(INCLUDEFLAGS) \
		   $(ARCH) -ffunction-sections -fdata-sections -specs=$(SPECS)

CFLAGS		+= -std=gnu17 -Wall $(DEFINES) $(INCLUDEFLAGS) \
		   $(ARCH) -O2 -ffunction-sections -fdata-sections -specs=$(SPECS)

LDFLAGS		:= $(ARCH) $(LIBDIRSFLAGS) -Wl,-Map,$(MAP),--gc-sections $(DEFINES) \
		   -Wl,--start-group $(LIBS) -lc -Wl,--end-group -specs=$(SPECS)

OBJS_SOURCES	:= $(addsuffix .o,$(addprefix $(BUILDDIR)/,$(SOURCES_S))) \
		   $(addsuffix .o,$(addprefix $(BUILDDIR)/,$(SOURCES_C)))

OBJS		:= $(OBJS_SOURCES)
DEPS		:= $(OBJS_SOURCES:.o=.d)

.PHONY: all clean

all: $(ELF)

$(ELF): $(OBJS)
	@echo "  LD.7    $@"
	$(V)$(LD) -o $@ $(OBJS) $(LDFLAGS)

clean:
	@echo "  CLEAN.7"
	$(V)$(RM) $(ELF) $(MAP) $(BUILDDIR)

$(BUILDDIR)/%.s.o : %.s
	@echo "  AS.7    $<"
	@$(MKDIR) -p $(@D)
	$(V)$(CC) $(ASFLAGS) -c $< -o $@

$(BUILDDIR)/%.c.o : %.c
	@echo "  CC.7    $<"
	@$(MKDIR) -p $(@D)
	$(V)$(CC) $(CFLAGS) -MMD -MP -c $< -o $@

-include $(DEPS)
