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

name    := $(basename $(notdir $(PWD)))
target  := $(shell uname -p)
objdir  :=.$(target)
sources := $(wildcard *.[cS])
objects := $(addprefix $(objdir)/, $(addsuffix .o, $(basename $(sources))))
opus    := $(objdir)/$(name)

#INC+=-I.
INC+=-I. -I../include -I../../include
#LIBS += -lm -lpthread
LIBS += ../../libq.b/$(objdir)/libq -lm -lpthread

# -E stop after preprocessor
# -pg -O -g -DUNOPT -DNDEBUG
# CFLAGS+=-g -O -Wall -Wstrict-prototypes -Werror
CFLAGS+=-g -Wall -Wstrict-prototypes -Werror \
	-D_F=\"$(basename $(notdir $(<)))\" \
	-D_LARGEFILE64_SOURCE -D_FILE_OFFSET_BITS=64 \
	$(.INCLUDES) $(INC) \

$(objdir)/%.o : %.c Makefile
	@ mkdir -p $(objdir)
	$(CC) $(CFLAGS) -c $< -o $@

$(objdir)/%.o : %.S Makefile
	@ mkdir -p $(objdir)
	$(CC) $(CFLAGS) -D__ASSEMBLY__ -c $< -o $@

$(opus):$(objects) $(LIBS)
	$(CC) $(CFLAGS) $(objects) $(LIBS) -o $(opus)
	cp $(opus) .

.PHONEY: install clean test

install:
	cp $(opus) ~/playbin

clean:
	@rm -fr $(objdir)
	@rm -f *.core
	@rm -f *.out

test:
	@ echo "sources=$(sources)"
	@ echo "objects=$(objects)"
	@ echo "CFLAGS=$(CFLAGS)"
