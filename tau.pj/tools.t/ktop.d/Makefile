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



#makedir := $(dir $(lastword $(MAKEFILE_LIST)))
makedir := ../../mk
include $(makedir)/gnu.mk

ifeq ($(BOARD),)
  TARGET = $(shell uname -m)
else
  TARGET = $(BOARD)
endif

a=$(shell rm -f xyzzy.c)

os	:= $(shell uname)
name    := $(basename $(notdir $(PWD)))
target  := $(TARGET)
objdir  :=.$(target)
sources := $(wildcard *.c)
headers := $(wildcard *.h)
objects := $(addprefix $(objdir)/, $(sources:.c=.o))
opus    := $(objdir)/$(name)
bin     ?= ~/playbin

include ../../mk/$(target).mk

INC+=-I. -I../include -I../../include

# -E stop after preprocessor
# -pg generate code fro gprof
# -O -g -DUNOPT -DNDEBUG
# CFLAGS+=-g -O -Wall -Wstrict-prototypes -Werror
CFLAGS += -rdynamic
CFLAGS += -static
CFLAGS +=-g -O -Wall -Wstrict-prototypes -Werror \
	-D_F=\"$(basename $(notdir $(<)))\" \
	-D_LARGEFILE64_SOURCE -D_FILE_OFFSET_BITS=64 \
	$(.INCLUDES) $(INC) \

LIBS += ../../libq.b/$(objdir)/libq
ifeq ($(os),Linux)
	LIBS += -lrt
endif
LIBS += -lm -lpthread -lncurses

objects += $(objdir)/syscall.o

$(opus):$(objects) $(LIBS)
	$(CC) $(CFLAGS) $(objects) $(LIBS) -o $(opus)
	cp $(opus) $(bin)

$(objdir)/%.o : %.c Makefile $(headers)
	@ mkdir -p $(objdir)
	$(CC) $(CFLAGS) -c $< -o $@

$(objdir)/syscall.o : $(objdir)/syscall.c /usr/include/sys/syscall.h
	$(CC) $(CFLAGS) -c $< -o $@

$(objdir)/syscall.c : /usr/include/sys/syscall.h Makefile syscall.awk
	@ mkdir -p $(objdir)
	echo '#include <sys/syscall.h>' | $(CC) -E -dD - -P | ./syscall.awk >$@

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
	@ echo "prefix=$(prefix)"
	@ echo "bin=$(bin)"
