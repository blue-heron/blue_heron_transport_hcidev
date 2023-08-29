# Makefile for building the NIF
#
# Makefile targets:
#
# all/install   build and install the NIF
# clean         clean build products and intermediates
#
# Variables to override:
#
# MIX_APP_PATH  path to the build directory
#
# CC            C compiler
# CROSSCOMPILE	crosscompiler prefix, if any
# CFLAGS	compiler flags for compiling all C files
# ERL_CFLAGS	additional compiler flags for files using Erlang header files
# ERL_EI_INCLUDE_DIR include path to ei.h (Required for crosscompile)
# ERL_EI_LIBDIR path to libei.a (Required for crosscompile)
# LDFLAGS	linker flags for linking all binaries
# ERL_LDFLAGS	additional linker flags for projects referencing Erlang libraries

PREFIX = $(MIX_APP_PATH)/priv
BUILD  = $(MIX_APP_PATH)/obj

CFLAGS ?= -O2 -Wall -Wextra -Wno-unused-parameter
CFLAGS += -std=c99 -D_GNU_SOURCE

SRC=$(wildcard src/*.c)

# Windows-specific updates
ifeq ($(OS),Windows_NT)
$(error "BlueHeronTransportHCIDev only works on Linux")
else
# Non-Windows

# -lrt is needed for clock_gettime() on linux with glibc before version 2.17
# (for example raspbian wheezy)
UNAME_S := $(shell uname -s)
ifeq ($(UNAME_S),Linux)
  LDFLAGS += -lrt
endif

# The paths to the EI library and header files are either passed in when
# compiled by Nerves (crosscompiled builds) or determined by mix.exs for
# host builds.
ifeq ($(ERL_EI_INCLUDE_DIR),)
$(error ERL_EI_INCLUDE_DIR not set. Invoke via mix)
endif
ifeq ($(ERL_EI_LIBDIR),)
$(error ERL_EI_LIBDIR not set. Invoke via mix)
endif

# Set Erlang-specific compile and linker flags
ERL_CFLAGS ?= -I$(ERL_EI_INCLUDE_DIR)
ERL_LDFLAGS ?= -L$(ERL_EI_LIBDIR) -lei

# If compiling on OSX and not crosscompiling, include CoreFoundation and IOKit
ifeq ($(CROSSCOMPILE),)
ifeq ($(shell uname),Darwin)
$(error "BlueHeronTransportHCIDev only works on Linux")
endif
endif

endif

HEADERS =$(wildcard src/*.h)
OBJ=$(SRC:src/%.c=$(BUILD)/%.o)
PORTEXE=$(PREFIX)/blue_heron_transport_hcidev

all: install

install: $(PREFIX) $(BUILD) $(PORTEXE)

$(OBJ): $(HEADERS) Makefile

$(BUILD)/%.o: src/%.c
	@echo " CC $(notdir $@)"
	$(CC) -c $(ERL_CFLAGS) $(CFLAGS) -o $@ $<

$(PORTEXE): $(OBJ)
	@echo " LD $(notdir $@)"
	$(CC) $^ $(ERL_LDFLAGS) $(LDFLAGS) -o $@


$(PREFIX) $(BUILD):
	mkdir -p $@

clean:
	$(RM) $(PORTEXE) $(OBJ)

.PHONY: all clean install

# Don't echo commands unless the caller exports "V=1"
${V}.SILENT:
