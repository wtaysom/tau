#! /usr/bin/env python

import re
import string
import subprocess

re_float = r"[+-]? *(?:\d+(?:\.\d*)?|\.\d+)(?:[eE][+-]?\d+)?"

def find_max(tag, text):
    r1 = re.search(tag + ".*\n(\d.*sec\n)+", text)
    r2 = re.findall(r"\d+\. (" + re_float + r") M.*\n", r1.group(0))
    print r2
    return max(float(result) for result in r2)


size = 4 * 1024
loops = 4
iterations = 4
cmd  = 'memcpy -z %d -i %d -l %d' % (size, iterations, loops)

subprocess.call(cmd + ' >memcpy.txt', shell=True)
f = open('memcpy.txt')
t = f.read()

print t

for tag in ['memcpy', '32bit', '64bit']:
    print tag, find_max(tag, t)
