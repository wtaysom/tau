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

##export INSTALL_MOD_DIR=updates

underscore:=_
empty:=
space:= $(empty) $(empty)

modules := $(wildcard *.k)
obj-m   := $(addsuffix /,$(modules))

objdir  :=.$(subst $(space),$(underscore),$(shell uname -srp))
pwd     := $(PWD)/$(objdir)
arch    := $(shell uname -i)
flavor  := $(shell uname -r | sed '-es/^.*-.*-//g')
kernel  := $(shell uname -r | sed '-es/-$(flavor)//g')
release := $(shell uname -r)
srcpath := /usr/src/linux
kdir    := $(srcpath)-$(kernel)
##kdir    := /lib/modules/$(release)/build

# Make sure the kdir path is there. If not, use /usr/src/linux
# but in that case, you must have built the kernel
ifneq ($(wildcard $(kdir)), $(kdir))
	kdir := $(srcpath)
endif

default:
	@echo $(modules)
	@mkdir -p $(objdir)
	@cp -Lpru $(modules) $(objdir)
	@echo "obj-m := $(obj-m)" >$(objdir)/Kbuild
	@make -C $(kdir) M=$(pwd) modules

install: default
	@cd $(objdir) && $(MAKE) -C $(kdir) M=$(pwd) modules_install
	@cd /lib/modules/$(release) && depmod

clean:
	rm -fr $(objdir)
