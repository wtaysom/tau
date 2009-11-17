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

name    := $(basename $(notdir $(src)))
sources := $(wildcard $(src)/*.c)
objects := $(sources:.c=.o)

kversion     := $(shell rpm -q kernel-source)
extraversion := $(word 6,$(shell echo $(kversion) | sed '-es/[-\.]/ /g'))


obj-m        := $(name).o
$(name)-objs := $(notdir $(objects))
EXTRA_CFLAGS += -Wall -Werror
EXTRA_CFLAGS += -D_F=\"$(*F)\"
EXTRA_CFLAGS += -DEXTRAVERSION=$(extraversion)
EXTRA_CFLAGS += -I$(src)
EXTRA_CFLAGS += -I$(src)/../../
EXTRA_CFLAGS += -I$(src)/../../../include
EXTRA_CFLAGS += -I$(src)/../../../../include
