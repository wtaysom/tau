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

CFLAGS += -DBLOCKNUM=$(BLOCKNUM)

include ../../mk/multi.mk
INC += -Iinclude
ifeq ($(os),Linux)
	LIBS += -luuid
endif
LIBS += -lm
LIBS += -lpthread
LIBS += -lncurses
