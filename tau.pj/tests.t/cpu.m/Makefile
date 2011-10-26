
#
# Copyright (c) 2011 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.
#

include ../../mk/multi.mk
LIBS += ../../libq.b/$(objdir)/libq -lm
ifeq ($(os),Linux)
	CFLAGS += -static
	LIBS += -lrt
endif
LIBS += -lpthread
