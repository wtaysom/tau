#! /usr/bin/env python
import re
import subprocess

file = 'xyzzy'
size = '0x20000'
loops = '4'
iterations = '1'

cmd = 'uread' + ' -f' + file + ' -z' + size + \
	' -i' + iterations + ' -l' +loops + ' -b12'

numerator = int(size, 0) * int(iterations)

subprocess.call(cmd + ' >uread.txt', shell=True)
f = open('uread.txt')
result = f.read()

#r1 = re.search("([^\s]*) MiB/s$", result)
r1 = re.search("timer avg=([^\s]*).*$", result)

timer_avg = float(r1.groups()[0])
p = int(size, 0) * int(iterations) / timer_avg / 1024.0 / 1024.0

# print r1
# p = r1.groups()[0]

print p

