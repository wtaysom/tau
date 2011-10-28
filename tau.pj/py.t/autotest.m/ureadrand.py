import re
import subprocess

file = 'xyzzy'
size = '0x200000000'
loops = '4'
iterations = '100000'

cmd = 'ureadrand' + ' -f' + file + ' -z' + size + \
	' -i' + iterations + ' -l' +loops + ' -b12'

numerator = 4096 * int(iterations)

subprocess.call(cmd + ' >ureadrand.txt', shell=True)
f = open('ureadrand.txt')
text = f.read()

r1 = re.search("timer avg=\d*.\d*.*$", text)
r2 = re.search("\d*\.\d*", r1.group(0))

print numerator / float(r2.group(0)) / 1024.0 / 1024.0

