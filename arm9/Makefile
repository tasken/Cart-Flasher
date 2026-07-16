# SPDX-License-Identifier: CC0-1.0
#
# SPDX-FileContributor: Antonio Niño Díaz, 2023

export BLOCKSDS			?= /opt/blocksds/core

LIBNCGC := $(CURDIR)/vendor/libncgc

BUILDDIR	:= build
SOURCEDIRS	:= source vendor/flashcart_core
INCLUDEDIRS	:= vendor/flashcart_core $(BUILDDIR) $(BUILDDIR)/data
DATA		:= data

DEFINES		:= -DARM9 \
		   -DCART_FLASHER_VERSION=\"$(CART_FLASHER_VERSION)\" \
		   -DCART_FLASHER_COMMIT=\"$(CART_FLASHER_COMMIT)\" \
		   -DNCGC_PLATFORM_NTR

ifneq ($(strip $(testing)),)
	DEFINES += -DTESTING=1
endif

LIBS		:= -lncgc -lstdc++ -lnds9
LIBDIRS		:= $(BLOCKSDS)/libs/libnds $(LIBNCGC)

NAME		:= arm9
ELF		:= $(CURDIR)/$(TARGET).elf
MAP		:= $(CURDIR)/$(TARGET).map

WONDERFUL_TOOLCHAIN	?= /opt/wonderful
ARM_NONE_EABI_PATH	?= $(WONDERFUL_TOOLCHAIN)/toolchain/gcc-arm-none-eabi/bin/
PREFIX		:= $(ARM_NONE_EABI_PATH)arm-none-eabi-
CC		:= $(PREFIX)gcc
CXX		:= $(PREFIX)g++
LD		:= $(PREFIX)gcc
RM		:= rm -rf
MKDIR		:= mkdir -p

ifeq ($(VERBOSE),1)
V		:=
else
V		:= @
endif

SOURCES_S	:= $(shell find -L $(SOURCEDIRS) -name "*.s")
SOURCES_C	:= $(shell find -L $(SOURCEDIRS) -name "*.c")
SOURCES_CPP	:= $(filter-out %example.cpp, $(shell find -L $(SOURCEDIRS) -name "*.cpp"))
SOURCES_BIN	:= $(shell find -L $(DATA) -name "*.*")

ARCH		:= -marm -mthumb-interwork -march=armv5te -mtune=arm946e-s
SPECS		:= $(BLOCKSDS)/sys/crts/ds_arm9.specs

INCLUDEFLAGS	:= $(foreach path,$(INCLUDEDIRS),-I$(path)) \
		   $(foreach path,$(LIBDIRS),-I$(path)/include)

LIBDIRSFLAGS	:= $(foreach path,$(LIBDIRS),-L$(path)/lib)

ASFLAGS		+= -x assembler-with-cpp $(DEFINES) $(INCLUDEFLAGS) \
		   $(ARCH) -ffunction-sections -fdata-sections -specs=$(SPECS)

CFLAGS		+= -std=gnu17 -Wall $(DEFINES) $(INCLUDEFLAGS) \
		   $(ARCH) -O2 -ffunction-sections -fdata-sections -specs=$(SPECS)

CXXFLAGS	+= -std=gnu++14 -Wall $(DEFINES) $(INCLUDEFLAGS) \
		   $(ARCH) -O2 -ffunction-sections -fdata-sections \
		   -fno-rtti -fno-exceptions -specs=$(SPECS)

LDFLAGS		:= $(ARCH) $(LIBDIRSFLAGS) -Wl,-Map,$(MAP),--gc-sections $(DEFINES) \
		   -Wl,--start-group $(LIBS) -lc -Wl,--end-group -specs=$(SPECS)

OBJS_BIN	:= $(addsuffix .o,$(addprefix $(BUILDDIR)/,$(SOURCES_BIN)))
HEADERS_BIN	:= $(addsuffix .h,$(subst .,_,$(addprefix $(BUILDDIR)/,$(SOURCES_BIN))))

OBJS_SOURCES	:= $(addsuffix .o,$(addprefix $(BUILDDIR)/,$(SOURCES_S))) \
		   $(addsuffix .o,$(addprefix $(BUILDDIR)/,$(SOURCES_C))) \
		   $(addsuffix .o,$(addprefix $(BUILDDIR)/,$(SOURCES_CPP)))

OBJS		:= $(OBJS_BIN) $(OBJS_SOURCES)
DEPS		:= $(OBJS_SOURCES:.o=.d)

.PHONY: all clean

all: $(ELF)

$(LIBNCGC)/lib/libncgc.a:
	@$(MAKE) PLATFORM=ntr -C $(LIBNCGC) fordka

$(ELF): $(LIBNCGC)/lib/libncgc.a $(OBJS)
	@echo "  LD.9    $@"
	$(V)$(LD) -o $@ $(OBJS) $(LDFLAGS)

clean:
	@echo "  CLEAN.9"
	$(V)$(MAKE) clean -C $(LIBNCGC)
	$(V)$(RM) $(ELF) $(MAP) $(BUILDDIR)

$(BUILDDIR)/%.s.o : %.s
	@echo "  AS.9    $<"
	@$(MKDIR) -p $(@D)
	$(V)$(CC) $(ASFLAGS) -c $< -o $@

$(BUILDDIR)/%.c.o : %.c
	@echo "  CC.9    $<"
	@$(MKDIR) -p $(@D)
	$(V)$(CC) $(CFLAGS) -MMD -MP -c $< -o $@

$(BUILDDIR)/%.cpp.o : %.cpp
	@echo "  CXX.9   $<"
	@$(MKDIR) -p $(@D)
	$(V)$(CXX) $(CXXFLAGS) -MMD -MP -c $< -o $@

# Rule for binary data files
$(BUILDDIR)/%.bin.o: %.bin
	@echo "  BIN.9   $<"
	@$(MKDIR) -p $(@D)
	$(V)$(BLOCKSDS)/tools/bin2c/bin2c $< $(@D)
	$(V)$(CC) $(CFLAGS) -c $(BUILDDIR)/$*_bin.c -o $@

-include $(DEPS)
