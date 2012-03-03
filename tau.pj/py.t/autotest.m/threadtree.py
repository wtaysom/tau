#! /usr/bin/env python

import os
import re
import subprocess

dir = '_Dir/'
iterations = 4
tasks = 2
width = 3
depth = 2
cmd = ('threadtree -d %s -i %d -t %d -w %d -k %d' %
	(dir, iterations, tasks, width, depth))

subprocess.call(cmd + ' >tree.txt', shell=True)
file = open('tree.txt')
result = file.read()

r1 = re.search(r"timer avg= *([^\s]*).*$", result)
print r1.groups()[0]
print r1.group(1)
timer_avg = float(r1.group(1))
p = tasks * pow(width, depth + 1) / timer_avg
print 'threadtree_ops', p
