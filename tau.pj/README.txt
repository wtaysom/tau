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

Tau is a techology not a product.
=================================

Disclaimer: This is not a commitment by Novell to provide any product
or service based on or related to anything in this project.

============================================================================

Suffix conventions:

Directories:
	.pj  project
	.t   sub tree - for now, restricted to children of .pj
	.b   library
	.d   normal process
	.m   multiple apps
	.k   kernel module
Files:
	.c   C file
	.h   header file
	.c.h a C source file treated like a header
		to handle files shared between
		kernel and user space.
	.txt text files like this one
	.mk  included makefiles
	.sh  shell script
