##===- source/Makefile -------------------------------------*- Makefile -*-===##
#
#                     The LLVM Compiler Infrastructure
#
# This file is distributed under the University of Illinois Open Source
# License. See LICENSE.TXT for details.
#
##===----------------------------------------------------------------------===##

LLDB_LEVEL := ..
include $(LLDB_LEVEL)/../../Makefile.config

DIRS :=

# enable lldb-gdbserver for supported platforms
ifneq (,$(strip $(filter $(HOST_OS), FreeBSD Linux GNU/kFreeBSD)))
DIRS += lldb-server
endif

ifeq ($(LLDB_ENABLE_DEBUGSERVER),1)
ifeq ($(HOST_OS),Darwin)
DIRS += debugserver
endif
endif

ifeq ($(ENABLE_WERROR),0)
DIRS += lldb-mi
endif

DIRS += driver

include $(LLDB_LEVEL)/Makefile
