#
############################################################################
# (C) Copyright 2008 Novell, Inc. All Rights Reserved.
#
#  GPLv2: This program is free software; you can redistribute it
#  and/or modify it under the terms of version 2 of the GNU General
#  Public License as published by the Free Software Foundation.
#
#  This program is distributed in the hope that it will be useful,
#  but WITHOUT ANY WARRANTY; without even the implied warranty of
#  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
#  GNU General Public License for more details.
############################################################################

#
# For this Makefile to work correctly, you must first
# configure and build the kernel.
#
# e.g.
#	cd /usr/src/linux
#	make cloneconfig
#	make
#
#KDIR	:= /usr/src/linux-$(shell uname -r)
#
#EXTRA_CFLAGS += -D_F=$(*F) # without suffix
#EXTRA_CFLAGS += -D_F=$(<F) # with .c extension
#EXTRA_CFLAGS += -D_F=\"$$(basename $$(notdir $$(<)))\"
#
underscore:=_
empty:=
space:= $(empty) $(empty)

name    := $(basename $(notdir $(PWD)))
target  := $(subst $(space),$(underscore),$(shell uname -srp))
objdir  :=.$(target)
sources := $(wildcard *.c)
objects := $(sources:.c=.o)

arch    := $(shell uname -i)
flavor  := $(shell uname -r | sed '-es/^.*-.*-//g')
srcpath := /usr/src/linux
sympath := $(srcpath)-obj
kdir    := $(sympath)/$(arch)/$(flavor)
release := $(shell uname -r)

kversion     := $(shell rpm -q kernel-source)
extraversion := $(word 6,$(shell echo $(kversion) | sed '-es/[-\.]/ /g'))
#extraversion := $(word 4,$(shell uname -r | sed '-es/[-\.]/ /g'))

ifneq ($(wildcard $(kdir)), $(kdir))
	kdir := $srcpath
endif

makefile := $(objdir)/Makefile
pwd      := $(PWD)/$(objdir)

default:
	@mkdir -p $(objdir)
	@cp -Lup $(sources) $(objdir)
	@echo "obj-m        := $(name).o" >$(makefile)
	@echo "$(name)-objs := $(objects)" >>$(makefile)
	@echo 'EXTRA_CFLAGS += -Wall -Werror' >>$(makefile)
	@echo 'EXTRA_CFLAGS += -D_F=\"$$(*F)\"' >>$(makefile)
	@echo 'EXTRA_CFLAGS += -DEXTRAVERSION=$(extraversion)' >>$(makefile)
	@echo 'EXTRA_CFLAGS += -I$$(src)/../' >>$(makefile)
	@echo 'EXTRA_CFLAGS += -I$$(src)/../../include' >>$(makefile)
	@echo 'EXTRA_CFLAGS += -I$$(src)/../../../include' >>$(makefile)
	@echo "default:" >>$(makefile)
	@echo '	$$(MAKE) -C $(kdir) M=$(pwd) modules' >>$(makefile)
	@cd $(objdir) && $(MAKE)

install: default
	@cd $(objdir) && $(MAKE) -C $(kdir) M=$(pwd) modules_install
	@cd /lib/modules/$(release) && depmod

clean:
	rm -fr $(objdir)
