import re

file = open('memcpy_test.txt')
text = file.read();

r1 = re.search("L1 cache.*\n.*\n.*", text)
r2 = re.search("\d+\.\d+ MiB/s$", r1.group(0))
print r2.group(0)

r1 = re.search("L2 cache.*\n.*\n.*", text)
r2 = re.search("\d+\.\d+ MiB/s$", r1.group(0))
print r2.group(0)

r1 = re.search("SDRAM.*\n.*\n.*", text)
r2 = re.search("\d+\.\d+ MiB/s$", r1.group(0))
print r2.group(0)
