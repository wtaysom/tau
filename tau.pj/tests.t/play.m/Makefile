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

BLOCKNUM?=32

CFLAGS += -DBLOCKNUM=$(BLOCKNUM) -rdynamic

system := $(shell uname)
include ../../mk/multi.mk
INC += -Iinclude
LIBS += ../../libq.b/$(objdir)/libq -lm
#LIBS += -lq -lm
#ifeq ($(system),Linux)
#	LIBS += -lrt -luuid
#endif
LIBS += -lrt
LIBS += -lpthread
#LIBS += -luuid
