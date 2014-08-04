#
# Main Makefile for the 12Sided Development Environment Build
#

#
# Copyright (c) 2013 12Sided Technology, LLC
#

libraries = tsl/

VERSION = 0
PATCHLEVEL = 1

NAME = A New Hope

#
# Avoid some Make BS
#
MAKEFLAGS += -rR --no-builtin-rules --no-print-directory

#
# Only language we care about is C
#
unexport LC_ALL
LC_COLLATE=C
LC_NUMERIC=C
export LC_COLLATE LC_NUMERIC

#
# Sometimes a little verbosity can't hurt...
#

ifeq ("$(origin V)", "command line")
	BUILD_VERBOSE = $(V)
endif
ifndef BUILD_VERBOSE
	BUILD_VERBOSE = 0
endif

#
# BUILD_SRC is not set on initial invocation
#
ifeq ($(BUILD_SRC),)

ifeq ($(origin B), "command line")
	BUILD_TYPE=$(B)
endif
ifeq ($(BUILD_TYPE),)
	BUILD_TYPE=debug
endif

export BUILD_TYPE

#
# Allow the user to tune the output of the build
#

ifeq ("$(origin O)", "command line")
	BUILD_OUTPUT = $(O)
endif

else

all_done := 1

endif # ifeq ($(BUILD_SRC),)

# If this is the final invocation of the file, invoke the makefile and exit
ifeq ($(all_done),)

#
# Get our bearings...
#
src_tree := $(if $(BUILD_SRC), $(BUILD_SRC), $(CURDIR))

include $(src_tree)/build/lib.mk

obj_tree := $(src_tree)
target_dir := $(src_tree)/$(BUILD_TYPE)

export src_tree obj_tree target_dir

VPATH := $(src_tree)

ARCH = x86_64 # TODO: actually support more than this?

export ARCH

# Specify the compilers and such
CC 			= gcc
CXX 		= g++
AS			= as
CPP			= $(CC) -E
AR			= ar
LD			= ld
RM			= rm -f

export CC CXX AS CPP AR LD RM

# Now, do we want to make the output pretty like a pony?
ifeq ($(BUILD_VERBOSE),1)
	quiet =
	Q =
else
	quiet = quiet_
	Q = @
endif

# Some people just can't bloody well help themselves...
ifneq ($(filter s% -s%,$(MAKEFLAGS)),)
	quiet=quiet_
endif

export quiet Q BUILD_VERBOSE

MAKEFLAGS += --include-dir="$(src_tree)"

DEFAULT_INCLUDES := $(src_tree)

DEFAULT_CFLAGS := -g -Wall -Wundef -Wstrict-prototypes -Wno-trigraphs \
					-fno-strict-aliasing -fno-common \
					-Werror-implicit-function-declaration \
					-Wno-format-security \
					-fno-delete-null-pointer-checks \
					-std=c99 -pthread
					# -march=corei7-avx -mtune=corei7-avx

DEFAULT_DEFINES := -D_ATS_IN_TREE -D_GNU_SOURCE -DSYS_CACHE_LINE_LENGTH=64

DEFAULT_ASFLAGS := -D__ASSEMBLY__ -march=corei7-avx -mtune=corei7-avx

DEFAULT_LDFLAGS := -L$(target_dir) -pthread -rdynamic -lrt -ldl -ljansson

INHERITED_CFLAGS :=

export DEFAULT_CFLAGS DEFAULT_DEFINES DEFAULT_ASFLAGS DEFAULT_INCLUDES
export INHERITED_CFLAGS DEFAULT_LDFLAGS

# TODO: move the subdirs somewhere less obnoxious
subdirs := $(libraries)

PHONY += all

all: target_dir $(subdirs)

PHONY += $(subdirs)

$(subdirs):
	$(Q)$(MAKE) $(build)=$@ $(MAKECMDGOALS)

PHONY += clean

clean: $(subdirs)

target_dir:
	mkdir -p $(target_dir)

PHONY += target_dir

endif # ifeq($(all_done),)

.PHONY: $(PHONY)

