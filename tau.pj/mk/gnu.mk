#
# Copyright (c) 2011 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.
#
# GNU's Makefile Conventions
#

SHELL = /bin/sh

prefix      = /usr/local
exec_prefix = $(prefix)
bindir      = $(exec_prefix)/bin/
sbindir     = $(exec_prefix)/sbin/
libexecdir  = $(exec_prefix)/libexec/

datarootdir    = $(prefix)/share/
datadir        = $(datarootdir)
sysconfdir     = $(prefix)/etc/
sharedstartdir = $(prefix)/com/
localstatedir  = $(prefix)/var/

includedir    = $(prefix)/incluide/
oldincludedir = /usr/include/

docdir  = $(datarootdir)/doc/$(package)/
infodir = $(datarootdir)/info/
htmldir = $(docdir)/
dvidir  = $(docdir)/
pdfdir  = $(docdir)/
psdir   = $(docdir)/

libdir   = $(exec_prefix)/lib/
lispdir  = $(datarootdir)/emacs/site-listp/
localdir = $(datarootdir)/locale/
mandir   = $(datarootdir)/man/
man1dir  = $(datarootdir)/man1/
man2dir  = $(datarootdir)/man2/
man3dir  = $(datarootdir)/man3/

srcdir = $(PWD)/
