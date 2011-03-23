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

TARGET  ?= $(shell uname -m)

makedir := $(dir $(lastword $(MAKEFILE_LIST)))
target  := $(TARGET)
objdir  :=.$(target)
sources := $(wildcard *.c)
objects := $(addprefix $(objdir)/, $(sources:.c=))
opuses  := $(sources:.c=)
bin     ?= ~/playbin

include $(makedir)/$(target).mk

INC+=-I. -I../include -I../../include

CFLAGS+=-O -g -Wall -Wstrict-prototypes -Werror \
	-D_F=\"$(basename $(notdir $(<)))\" \
	-D_LARGEFILE64_SOURCE -D_FILE_OFFSET_BITS=64 \
	$(.INCLUDES) $(INC) \

all: $(objects)

$(objdir)/% : %.c
	@ mkdir -p $(objdir)
	echo $(LIBS)
	$(CC) $(CFLAGS) -o $@ $^ $(LIBS)
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
	@echo "objdir ="$(objdir)
	@echo "objects="$(objects)
	@echo "opuses ="$(opuses)
