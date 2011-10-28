import re
import string
import subprocess

# size = '0x4000000'
size = '0x400'
loops = '4'
# iterations = '10'
iterations = '2'

cmd = 'memcpy' + ' -z' + size + ' -i' + iterations + ' -l' +loops

subprocess.call(cmd + ' >memcpy.txt', shell=True)
f = open('memcpy.txt')
t = f.read()

def find_max(tag, text):
    r1 = re.search(tag + ".*\n(\d.*\n)+", text)
    r2 = re.search(r"(\d+\. \d+\.\d+.*\n)+", r1.group(0))
    runs = string.split(r2.group(0), '\n')
    max = 0.0
    for r in runs:
        a = re.search('\d+.\d+', r)
        if (a):
            b = float(a.group(0))
            if (b > max):
                max = b
    return max

for tag in ['memcpy', '32bit', '64bit']:	
    print tag, find_max(tag, t)
