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
	print "#include <style.h>\n"
	print "const char *const Syscall[] = {"
}
/define[ \t]+__NR_/ {	gsub("__NR_", "", $2)
			printf "\t\"%s\",\n", $2 }
END {	print "};\n"
	print "const int Num_syscalls = sizeof(Syscall) / sizeof(*Syscall);"
	print "#endif"
}
