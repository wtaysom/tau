#!/usr/bin/awk -f
#
# Copyright (c) 2011 The Chromium OS Authors. All rights reserved.
# Distributed under the terms of the GNU General Public License v2
#
BEGIN { print "/*"
	print " * Copyright (c) 2011 The Chromium OS Authors. All rights reserved."
	print " * Distributed under the terms of the GNU General Public License v2"
	print " */\n"
	print "#ifndef _SYSCALL_H_"
	print "#define _SYSCALL_H_ 1\n"
	print "const char *const Syscall[] = {"
}
{ printf "\t\"%s\",", $2 }
END {	print "};\n"
	print "const int Num_syscalls = sizeof(Syscall) / sizeof(char *);"
	print "#endif"
}
