import re
import subprocess

file = 'xyzzy'
size = '0x200000000'
loops = '4'
iterations = '1'

cmd = 'uread' + ' -f' + file + ' -z' + size + \
	' -i' + iterations + ' -l' +loops + ' -b12'

numerator = int(size, 0) * int(iterations)

subprocess.call(cmd + ' >uread.txt', shell=True)
f = open('uread.txt')
text = f.read()

r1 = re.search("timer avg=\d*.\d*.*$", text)
r2 = re.search("\d*\.\d*", r1.group(0))

print numerator / float(r2.group(0)) / 1024.0 / 1024.0

