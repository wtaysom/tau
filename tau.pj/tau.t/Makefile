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
# $Author$
# $Date$
# $Revision$
############################################################################

CFS_SUBDIRS :=	kernel.m\
		cmd.d \
		dirfs.d \
		mkfs.bag.d \
		mkfs.vol.d \
		sage.d \
		sw.d \
		tests.m \
		watchdog.d \

default:
	@for i in $(CFS_SUBDIRS); do \
		(cd $$i && $(MAKE)) || exit 1; \
	done

install:
	@for i in $(CFS_SUBDIRS); do \
		(cd $$i && $(MAKE) install) || exit 1; \
	done


clean:
	@for i in $(CFS_SUBDIRS); do \
		(cd $$i && $(MAKE) clean) || exit 1; \
	done

