#! /usr/bin/env python
import re
import subprocess

re_float = r"[+-]? *(?:\d+(?:\.\d*)?|\.\d+)(?:[eE][+-]?\d+)?"

file = 'xyzzy'
size = 8 * 1024
loops = 4
iterations = 1
cmd = ('uread -f %s -z %d -i %d -l %d -b12' %
	(file, size, iterations, loops))

subprocess.call(cmd + ' >uread.txt', shell=True)
f = open('uread.txt')
result = f.read()

r1 = re.search(r"[^\s]+ MiB/s.*$", result)
print r1.group(0)
r2 = re.search(re_float, r1.group(0))
mib_s = r2.group(0)
print 'uread_MiB_s', mib_s
