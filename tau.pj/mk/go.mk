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

include ../../mk/Make.inc


name    := $(basename $(notdir $(PWD)))
objdir  :=.$(GOOS)/$(GOARCH)
sources := $(wildcard *.go)
objects := $(addprefix $(objdir)/, $(sources:.go=.$O))
opus    := $(objdir)/$(name)
bin     ?= ~/playbin

$(objdir)/%.$O : %.go Makefile
	@ mkdir -p $(objdir)
	$(GC) -I $(objdir) -o $@ $<

$(opus):$(objects) $(LIBS)
	$(LD) -L $(objdir) -o $(opus) $(objdir)/main.$O $(LIBS)
	cp $(opus) $(bin)

.PHONEY: install clean test

install:
	cp $(opus) $(bin)

clean:
	@rm -fr $(objdir)
	@rm -f *.core
	@rm -f *.out
	@rm -f $(bin)/$(opus)

test:
	@ echo "sources=$(sources)"
	@ echo "objects=$(objects)"
