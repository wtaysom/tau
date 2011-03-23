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
os	:= $(shell uname)
opus    := $(basename $(notdir $(PWD)))
target  := $(TARGET)
objdir  :=.$(target)
sources := $(wildcard *.c)
objects := $(addprefix $(objdir)/, $(sources:.c=.o))

include $(makedir)/$(target).mk

#CC=/usr/local/bin/gcc
INC+=-I. -I../include -I../../include

# -pg -O -g -DUNOPT -DNDEBUG
CFLAGS+=-O -pg -g -Wall -Wstrict-prototypes -Werror \
	-D_LARGEFILE64_SOURCE -D_FILE_OFFSET_BITS=64 \
	$(.INCLUDES) $(INC) \

$(objdir)/%.o : %.c Makefile
	@ mkdir -p $(objdir)
	$(CC) $(CFLAGS) -c $< -o $@

$(opus): $(objects)
	@rm -f $(objdir)/$@
	@case $(os) in \
		Linux)	echo "Building linux library $@"; \
			ar rc $(objdir)/$@ $(objects); \
			;; \
		Darwin)	echo "Buidling apple library $@"; \
			libtool -static -o $(objdir)/$@ $(objects); \
			;; \
	esac

install:
	cp $(objdir)/$(opus) /tmp/$(opus).a
	sudo mv /tmp/$(opus).a /usr/lib/$(opus).a

clean:
	@rm -f *.core
	@rm -f *.out
	@rm -fr $(objdir)
