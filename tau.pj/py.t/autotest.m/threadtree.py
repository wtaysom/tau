import re
import subprocess

iterations = '4'
tasks = '2'
width = '3'
depth = '5'

cmd = 'threadtree' + ' -w' + width + ' -k' + depth + ' -t' + tasks + \
    ' -i' + iterations

numerator = int(tasks) * pow(int(width), int(depth) + 1)

subprocess.call(cmd + ' >tree.txt', shell=True)
file = open('tree.txt')
text = file.read()

r1 = re.search("timer avg=\d*.\d*.*$", text)
r2 = re.search("\d*\.\d*", r1.group(0))

print numerator / float(r2.group(0))

