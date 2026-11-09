#---------------------------------------------------------------------------------
# DangoPlayer Switch - Makefile
# Based on devkitPro Nintendo Switch homebrew template
#---------------------------------------------------------------------------------
.SUFFIXES:
#---------------------------------------------------------------------------------

ifeq ($(strip $(DEVKITPRO)),)
$(error "Please set DEVKITPRO in your environment. export DEVKITPRO=<path to>/devkitpro")
endif

TOPDIR ?= $(CURDIR)
include $(DEVKITPRO)/libnx/switch_rules

#---------------------------------------------------------------------------------
# Project configuration
#---------------------------------------------------------------------------------
TARGET		:=	PassionWiiStreamingSwitch
BUILD		:=	build
SOURCES		:=	source source/ui source/ui/widgets source/media source/network source/utils
DATA		:=	data
INCLUDES	:=	include include/ui include/ui/widgets include/media include/network include/utils
ROMFS		:=	romfs

#---------------------------------------------------------------------------------
# Application metadata (shown in Homebrew Menu & Forwarders)
#---------------------------------------------------------------------------------
export APP_TITLE	:=	PassionWiiStreaming Switch Edition
export APP_AUTHOR	:=	PassionWii Team
export APP_VERSION	:=	1.0.0

#---------------------------------------------------------------------------------
# Compiler flags
#---------------------------------------------------------------------------------
ARCH	:=	-march=armv8-a+crc+crypto -mtune=cortex-a57 -mtp=soft -fPIE

CFLAGS	:=	-g -Wall -O2 -ffunction-sections \
			$(ARCH) $(DEFINES)

CFLAGS	+=	$(INCLUDE) -D__SWITCH__

CXXFLAGS	:=	$(CFLAGS) -std=gnu++17 -fno-rtti -fno-exceptions

ASFLAGS	:=	-g $(ARCH)
LDFLAGS	=	-specs=$(DEVKITPRO)/libnx/switch.specs -g $(ARCH) -Wl,-Map,$(notdir $*.map)

#---------------------------------------------------------------------------------
# Libraries to link (order matters; wrapped in start/end group for circular deps)
#---------------------------------------------------------------------------------
LIBS	:=	-Wl,--start-group \
			-lmpv -lavfilter -lavformat -lavcodec -lavutil -lswscale -lswresample \
			-lplacebo -llua -lass -lfribidi -ldav1d \
			-lcurl -lmbedtls -lmbedcrypto -lmbedx509 \
			-lSDL2_ttf -lSDL2_image -lSDL2_gfx -lSDL2_mixer \
			-lSDL2 -lglad -lEGL -lGLESv2 -lglapi -ldrm_nouveau \
			-lfreetype -lharfbuzz -lpng16 -ljpeg -lwebp \
			-lbz2 -lz -lm -lnx \
			-Wl,--end-group

#---------------------------------------------------------------------------------
# Library search paths
#---------------------------------------------------------------------------------
LIBDIRS	:=	$(PORTLIBS) $(LIBNX)

#---------------------------------------------------------------------------------
# Build rules — do not edit below this line unless adding custom rules
#---------------------------------------------------------------------------------
ifneq ($(BUILD),$(notdir $(CURDIR)))
#---------------------------------------------------------------------------------

export OUTPUT	:=	$(CURDIR)/$(TARGET)
export TOPDIR	:=	$(CURDIR)

export VPATH	:=	$(foreach dir,$(SOURCES),$(CURDIR)/$(dir)) \
					$(foreach dir,$(DATA),$(CURDIR)/$(dir))

export DEPSDIR	:=	$(CURDIR)/$(BUILD)

CFILES		:=	$(foreach dir,$(SOURCES),$(notdir $(wildcard $(dir)/*.c)))
CPPFILES	:=	$(foreach dir,$(SOURCES),$(notdir $(wildcard $(dir)/*.cpp)))
SFILES		:=	$(foreach dir,$(SOURCES),$(notdir $(wildcard $(dir)/*.s)))
BINFILES	:=	$(foreach dir,$(DATA),$(notdir $(wildcard $(dir)/*.*)))

ifneq ($(strip $(CPPFILES)),)
	export LD	:=	$(CXX)
else
	export LD	:=	$(CC)
endif

export OFILES_BIN	:=	$(addsuffix .o,$(BINFILES))
export OFILES_SRC	:=	$(CPPFILES:.cpp=.o) $(CFILES:.c=.o) $(SFILES:.s=.o)
export OFILES		:=	$(OFILES_BIN) $(OFILES_SRC)
export HFILES_BIN	:=	$(addsuffix .h,$(subst .,_,$(BINFILES)))

export INCLUDE	:=	$(foreach dir,$(INCLUDES),-I$(CURDIR)/$(dir)) \
					$(foreach dir,$(LIBDIRS),-I$(dir)/include) \
					-I$(CURDIR)/$(BUILD)

export LIBPATHS	:=	$(foreach dir,$(LIBDIRS),-L$(dir)/lib)

# Include RomFS data in the NRO
ifneq ($(ROMFS),)
	export NROFLAGS	+=	--romfsdir=$(CURDIR)/$(ROMFS)
endif

# Use custom icon if present (place icon.jpg in project root)
ifneq ($(wildcard $(CURDIR)/icon.jpg),)
	export NROFLAGS	+=	--icon=$(CURDIR)/icon.jpg
endif

# Embed NACP metadata into the NRO (required for Sphaira forwarder installer and Home Menu)
ifeq ($(strip $(NO_NACP)),)
	export NROFLAGS	+=	--nacp=$(CURDIR)/$(TARGET).nacp
endif

.PHONY: $(BUILD) clean all

all: $(BUILD)

$(BUILD):
	@[ -d $@ ] || mkdir -p $@
	@$(MAKE) --no-print-directory -C $(BUILD) -f $(CURDIR)/Makefile

clean:
	@echo clean ...
	@rm -fr $(BUILD) $(TARGET).nro $(TARGET).nacp $(TARGET).elf

#---------------------------------------------------------------------------------
else
.PHONY: all

DEPENDS	:=	$(OFILES:.o=.d)

all: $(OUTPUT).nro

$(OUTPUT).nro	:	$(OUTPUT).elf $(OUTPUT).nacp
$(OUTPUT).elf	:	$(OFILES)

$(OFILES_SRC)	: $(HFILES_BIN)

#---------------------------------------------------------------------------------
# Binary data rules
#---------------------------------------------------------------------------------
%.bin.o	%_bin.h :	%.bin
	@echo $(notdir $<)
	@$(bin2o)

-include $(DEPENDS)

#---------------------------------------------------------------------------------
endif
#---------------------------------------------------------------------------------
