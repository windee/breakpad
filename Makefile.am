## Process this file with automake to produce Makefile.in

# Copyright (c) 2011, Google Inc.
# All rights reserved.
#
# Redistribution and use in source and binary forms, with or without
# modification, are permitted provided that the following conditions are
# met:
#
#     * Redistributions of source code must retain the above copyright
# notice, this list of conditions and the following disclaimer.
#     * Redistributions in binary form must reproduce the above
# copyright notice, this list of conditions and the following disclaimer
# in the documentation and/or other materials provided with the
# distribution.
#     * Neither the name of Google Inc. nor the names of its
# contributors may be used to endorse or promote products derived from
# this software without specific prior written permission.
#
# THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
# "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
# LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
# A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
# OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
# SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
# LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
# DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
# THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
# (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
# OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.


# This allows #includes to be relative to src/
AM_CPPFLAGS = -I$(top_srcdir)/src
AM_CFLAGS =
AM_CXXFLAGS =


AM_CXXFLAGS += $(WARN_CXXFLAGS)

# Specify include paths for ac macros
ACLOCAL_AMFLAGS = -I m4

# License file is called LICENSE not COPYING
AUTOMAKE_OPTIONS = foreign

## Documentation
docdir = $(prefix)/share/doc/$(PACKAGE)-$(VERSION)

## Headers

includegbcdir = $(includedir)/$(PACKAGE)/google_breakpad/common
includegbc_HEADERS = $(top_srcdir)/src/google_breakpad/common/*.h



includepdir = $(includedir)/$(PACKAGE)/processor
includep_HEADERS = $(top_srcdir)/src/processor/*.h


## Libraries
check_LIBRARIES =
noinst_LIBRARIES =
lib_LIBRARIES =
libexec_PROGRAMS =
bin_PROGRAMS =
check_PROGRAMS =
EXTRA_PROGRAMS =
CLEANFILES =

if !DISABLE_PROCESSOR
noinst_LIBRARIES += src/third_party/libdisasm/libdisasm.a
endif

src_third_party_libdisasm_libdisasm_a_SOURCES = \
	src/third_party/libdisasm/ia32_implicit.c \
	src/third_party/libdisasm/ia32_implicit.h \
	src/third_party/libdisasm/ia32_insn.c \
	src/third_party/libdisasm/ia32_insn.h \
	src/third_party/libdisasm/ia32_invariant.c \
	src/third_party/libdisasm/ia32_invariant.h \
	src/third_party/libdisasm/ia32_modrm.c \
	src/third_party/libdisasm/ia32_modrm.h \
	src/third_party/libdisasm/ia32_opcode_tables.c \
	src/third_party/libdisasm/ia32_opcode_tables.h \
	src/third_party/libdisasm/ia32_operand.c \
	src/third_party/libdisasm/ia32_operand.h \
	src/third_party/libdisasm/ia32_reg.c \
	src/third_party/libdisasm/ia32_reg.h \
	src/third_party/libdisasm/ia32_settings.c \
	src/third_party/libdisasm/ia32_settings.h \
	src/third_party/libdisasm/libdis.h \
	src/third_party/libdisasm/qword.h \
	src/third_party/libdisasm/x86_disasm.c \
	src/third_party/libdisasm/x86_format.c \
	src/third_party/libdisasm/x86_imm.c \
	src/third_party/libdisasm/x86_imm.h \
	src/third_party/libdisasm/x86_insn.c \
	src/third_party/libdisasm/x86_misc.c \
	src/third_party/libdisasm/x86_operand_list.c \
	src/third_party/libdisasm/x86_operand_list.h

## Programs
bin_PROGRAMS += \
	src/processor/minidump_stackwalk

## Non-installables
noinst_PROGRAMS =
noinst_SCRIPTS = $(check_SCRIPTS)

src_processor_minidump_stackwalk_SOURCES = \
	src/processor/minidump_stackwalk.cc
src_processor_minidump_stackwalk_LDADD = \
	src/processor/path_helper.o \
	src/processor/basic_code_modules.o \
	src/processor/basic_source_line_resolver.o \
	src/processor/call_stack.o \
	src/processor/cfi_frame_info.o \
	src/processor/convert_old_arm64_context.o \
	src/processor/disassembler_x86.o \
	src/processor/dump_context.o \
	src/processor/dump_object.o \
	src/processor/exploitability.o \
	src/processor/exploitability_linux.o \
	src/processor/exploitability_win.o \
	src/processor/logging.o \
	src/processor/minidump.o \
	src/processor/minidump_processor.o \
	src/processor/pathname_stripper.o \
	src/processor/process_state.o \
	src/processor/proc_maps_linux.o \
	src/processor/simple_symbol_supplier.o \
	src/processor/source_line_resolver_base.o \
	src/processor/stack_frame_cpu.o \
	src/processor/stack_frame_symbolizer.o \
	src/processor/stackwalk_common.o \
	src/processor/stackwalker.o \
	src/processor/stackwalker_address_list.o \
	src/processor/stackwalker_amd64.o \
	src/processor/stackwalker_arm.o \
	src/processor/stackwalker_arm64.o \
	src/processor/stackwalker_mips.o \
	src/processor/stackwalker_ppc.o \
	src/processor/stackwalker_ppc64.o \
	src/processor/stackwalker_sparc.o \
	src/processor/stackwalker_x86.o \
	src/processor/symbolic_constants_win.o \
	src/processor/tokenize.o \
	src/third_party/libdisasm/libdisasm.a


mostlyclean-local:
	-find src -name '*.dwo' -exec rm -f {} +
