#
# A build system inspired from KBuild -- sort of.
#
# Initialize variables we don't want to grab from the env

tgt_dir = $(src_tree)/$(tgt)

# Fields that can be/must be set in the 12Build file
TARGET_LDFLAGS :=
TARGET_CFLAGS :=
TARGET_CXXFLAGS :=
TARGET_TYPE :=
TARGET_DEFINES :=
OUTPUT :=
SUBDIRS :=
SUBDIRS_BUILD :=
TARGET :=
OBJ :=
LIBS :=

# Fields that are pulled in from the target type
OFLAGS :=
TYPE_DEFINES :=

include $(src_tree)/build/lib.mk

include $(tgt_dir)12Build

headers = $(wildcard $(tgt_dir)*.h)

ifeq ($(BUILD_TYPE),)
	$(error Build type not specified!)
endif

bld-obj := $(addprefix $(tgt_dir),$(OBJ))
includes := $(addprefix -I, $(DEFAULT_INCLUDES))
targ-defines := $(addprefix -D, $(TARGET_DEFINES))
lib-line = $(addprefix -l, $(LIBS))

include $(src_tree)/build/$(BUILD_TYPE).mk

ifeq ($(TARGET_TYPE),static)
	TARGET_LDFLAGS=
	TARGET_CFLAGS=$(DEFAULT_CFLAGS)
	# Make sure the static library is named properly
	OUTPUT=$(target_dir)/lib$(TARGET).a
else ifeq ($(TARGET_TYPE),shared)
	libname=lib$(TARGET).so
	liblinkpath=$(target_dir)/$(libname)
	liblinkverpath=$(liblinkpath).$(TARGET_MAJOR)
	# Populate SONAME correctly
	TARGET_LDFLAGS=-shared -Wl,-soname,$(libname).$(TARGET_MAJOR)
	# We need to build with PIC for a shared object
	TARGET_CFLAGS=$(DEFAULT_CFLAGS) -fPIC
	# Our target name needs to be prefixed with lib, and the target versions
	OUTPUT=$(target_dir)/$(libname).$(TARGET_MAJOR).$(TARGET_MINOR)
	INHERITED_CFLAGS = -fPIC
	export INHERITED_CFLAGS
else ifeq ($(TARGET_TYPE),app)
	TARGET_LDFLAGS=$(DEFAULT_LDFLAGS) $(lib-line)
	TARGET_CFLAGS=$(DEFAULT_CFLAGS)
	OUTPUT=$(target_dir)/$(TARGET).exe
else ifeq ($(TARGET_TYPE),group)
	PHONY += fake_target
	TARGET = fake_target
else ifeq ($(TARGET_TYPE),)
	TARGET_LDFLAGS = $(DEFAULT_LDFLAGS)
	TARGET_CFLAGS = $(DEFAULT_CFLAGS) $(INHERITED_CFLAGS)
	OUTPUT=$(tgt_dir)built-in.o
	TARGET=$(OUTPUT)
endif # ifeq($(TARGET_TYPE),"static")

.SUFFIXES:
.SUFFIXES: .c .cpp .o

TARGET_CFLAGS += $(includes) $(DEFAULT_DEFINES) $(targ-defines) $(OFLAGS) $(TYPE_DEFINES)
TARGET_LDFLAGS += $(OFLAGS)

subdir-builtin := $(addsuffix built-in.o, $(addprefix $(tgt_dir), $(SUBDIRS)))
subdirs := $(addprefix $(tgt), $(SUBDIRS))
subdirs-bld := $(addprefix $(tgt), $(SUBDIRS_BUILD))

link-obj = $(bld-obj) $(subdir-builtin)

export TARGET_LDFLAGS TARGET_CFLAGS

# Helpers for untold evil
comma   := ,
squote  := '
# '
empty   :=
space   := $(empty) $(empty)

out_of_tree = $(empty)   $(empty)

# Escape a single quote
escsq = $(subst $(squote),'\$(squote)',$1)

echo-cmd = $(if $($(quiet)cmd_$(1)),\
	@echo '  $(call escsq,$($(quiet)cmd_$(1)))';)

# Quiet invocation of CC
quiet_cmd_cc_o_c = CC  $(out_of_tree)  $@
      cmd_cc_o_c = $(CC) $(TARGET_CFLAGS) -c -o $@ $<

$(tgt-dir)/%.o : $(tgt-dir)/%.c
	$(call echo-cmd,cc_o_c) $(cmd_cc_o_c)

# Quiet invocation of CXX
quiet_cmd_cxx_o_cpp = CXX $(out_of_tree) $@
      cmd_cxx_o_cpp = $(CXX) $(TARGET_CXXFLAGS) -c -o $@ $<

$(tgt-dir)/%.o : $(tgt-dir)/%.cpp
	$(call echo-cmd,cxx_o_cpp) $(cmd_cxx_o_cpp)

# Quiet invocation of AR
quiet_cmd_gen_archive = AR  $(out_of_tree) $(OUTPUT)
      cmd_gen_archive = $(AR) rcs $(OUTPUT) $(link-obj)

cmd_do_static_link = $(call echo-cmd,gen_archive) $(cmd_gen_archive)

quiet_cmd_shared_link = LDS $(out_of_tree) $(OUTPUT)
      cmd_shared_link = $(CC) -o $(OUTPUT) $(link-obj) $(TARGET_LDFLAGS)

cmd_do_shared_link = $(call echo-cmd,shared_link) $(cmd_shared_link)

quiet_cmd_shared_symlink = SYM $(out_of_tree) $1
      cmd_shared_symlink = ln -sf $(OUTPUT) $1

cmd_do_shared_symlink = $(call echo-cmd,shared_symlink) $(cmd_shared_symlink)

cmd_rm_symlink = $(RM) $1

quiet_cmd_app_link = LD  $(out_of_tree) $(OUTPUT)
	  cmd_app_link = $(CC) -o $(OUTPUT) $(link-obj) $(TARGET_LDFLAGS)

cmd_do_app_link = $(call echo-cmd,app_link) $(cmd_app_link)

# Do a composite link of all objects in the directory (for child targets w/o a type)
quiet_cmd_composite_link = LD  $(out_of_tree) $(OUTPUT)
	  cmd_composite_link = $(LD) -r -o $(OUTPUT) $(link-obj)

cmd_do_composite_link := $(call echo-cmd,composite_link) $(cmd_composite_link)

cmd_do_link :=
ifeq ($(TARGET_TYPE),static)
	cmd_do_link := $(cmd_do_static_link)
else ifeq ($(TARGET_TYPE),shared)
	cmd_do_link := $(cmd_do_shared_link)
else ifeq ($(TARGET_TYPE),app)
	cmd_do_link := $(cmd_do_app_link)
else ifeq ($(TARGET_TYPE),group)
	cmd_do_link :=
else
	cmd_do_link := $(cmd_do_composite_link)
endif

quite_cmd_clean_item = CLEAN  $@
      cmd_clean_item = $(RM) $@

PHONY += all

all: $(bld-obj) $(subdirs) $(OUTPUT) $(subdirs-bld) $(headers)

$(OUTPUT): .FORCE
	$(call cmd_do_link)
	$(if $(filter $(TARGET_TYPE),shared), $(call cmd_shared_symlink,$(liblinkpath)),)
	$(if $(filter $(TARGET_TYPE),shared), $(call cmd_shared_symlink,$(liblinkverpath)),)

# Recurse through specified subdirectories
PHONY += $(subdirs) $(subdirs-bld)

$(subdirs):
	$(Q)$(MAKE) $(build)=$@ $(MAKECMDGOALS)

$(subdirs-bld):
	$(Q)$(MAKE) $(build)=$@ $(MAKECMDGOALS)

PHONY += clean

clean: $(subdirs) $(subdirs-bld)
	$(RM) $(bld-obj) $(OUTPUT)
	$(if $(filter $(target_type),shared), $(call cmd_rm_symlink,$(liblinkpath)),)
	$(if $(filter $(target_type),shared), $(call cmd_rm_symlink,$(liblinkverpath)),)

.PHONY: $(PHONY)

.FORCE:

