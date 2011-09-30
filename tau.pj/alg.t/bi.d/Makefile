
#
# Copyright (c) 2011 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.
#

include ../../mk/default.mk
LIBS += ../../libq.b/$(objdir)/libq
ifeq ($(os),Linux)
  LIBS += -lrt
endif
LIBS += -lm

run:$(opus)
	$(opus) -i 1000
