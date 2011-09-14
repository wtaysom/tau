
#
# Copyright (c) 2011 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.
#

CFLAGS += -static

include ../../mk/multi.mk
LIBS += ../../libq.b/$(objdir)/libq -lm -lpthread
ifeq ($(os),Linux)
  LIBS += -lrt
endif
