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
# Old things I don't want to forget
##target  := $(shell uname -m)


name    := $(basename $(notdir $(PWD)))
target  := $(TARGET)
objdir  :=.$(target)
sources := $(wildcard *.c)
objects := $(addprefix $(objdir)/, $(sources:.c=.o))
opus    := $(objdir)/$(name)
bin     ?= ~/playbin

include $(BASEPJ)/mk/$(target).mk

INC+=-I. -I../include -I../../include

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

$(opus):$(objects) $(LIBS)
	$(CC) $(CFLAGS) $(objects) $(LIBS) -o $(opus)
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
	@ echo "CFLAGS=$(CFLAGS)"
