file = open('memcpy_test.txt')
text=file.read();
r = re.search("L1 cache.*\n.*\n.*", text)
r2=re.search("\d+\.\d+ MiB/s$", r.group(0))
print r2.group(0)

