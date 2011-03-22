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

include ../../mk/Make.$(GOARCH)

objdir  :=.$(GOOS)/$(GOARCH)
sources := $(wildcard *.go)
objects := $(addprefix $(objdir)/, $(sources:.go=))
opuses  := $(sources:.go=)
bin     ?= ~/playbin

all: $(objects)

$(objdir)/% : %.go
	@ mkdir -p $(objdir)
	$(GC) -o $@.$O $^
	$(LD) -o $@ $@.$O
	cp $@ $(bin)

install:
	cd $(objdir); cp $(opuses) $(bin)

.PHONEY : clean
clean:
	@ rm -fr $(objdir)
	@rm -f *.core
	@rm -f *.out
	@ cd $(bin) ; rm -f $(opuses)

cleanbin:
	@ cd $(bin) ; rm -f $(opuses)

test:
	@echo $(opuses)
	@echo $(objdir)
	@echo $(sources)
	@echo $(objects)

