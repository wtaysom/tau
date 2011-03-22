/*
 * Copyright (c) 2010 The Chromium OS Authors. All rights reserved.
 * Distributed under the terms of the GNU General Public License v2
 */
#include "eprintf.h"

int main (int argc, char *argv[]) {
        eprintf("This failed");
        return 0;
}
