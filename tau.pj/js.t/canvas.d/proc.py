#!/usr/bin/python
#
# Copyright (c) 2011 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import string

with open('/proc/stat') as file:
  cpu = string.split(file.readline())
  print cpu[1]

