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
release := $(shell uname -r)
kdir    := /lib/modules/$(release)/build

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
