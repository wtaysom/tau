#! /usr/bin/env python
import re
import subprocess

re_float = r"[+-]? *(?:\d+(?:\.\d*)?|\.\d+)(?:[eE][+-]?\d+)?"

file = 'xyzzy'
size = 8 * 1024
loops = 4
iterations = 1
cmd = ('uwrite -f %s -z %d -i %d -l %d -b12' %
	(file, size, iterations, loops))

subprocess.call(cmd + ' >uwrite.txt', shell=True)
f = open('uwrite.txt')
result = f.read()

r1 = re.search(r"avg=.*$", result)
print r1.group(0)
r2 = re.search(re_float, r1.group(0))
secs = r2.group(0)
mib_s = size * iterations / float(secs) / (1024 * 1024)
print 'uwrite_MiB_s', mib_s
