#
# Copyright (c) 2011 The Chromium OS Authors. All rights reserved.
# Distributed under the terms of the GNU General Public License v2 
# 
# Used to experiment with makefiles.

makedir := $(dir $(lastword $(MAKEFILE_LIST)))

default:
	@echo "makedir="$(makedir)
