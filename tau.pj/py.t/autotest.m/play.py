import re

m = re.match(r"(\w+) (\w+)", "Isaac Newton, physicist")

print m.group(0)
print m.group(1)
print m.group(2)

m = re.search(r"(\d )+(\d)", "1 2 3 4")

print m.group(0)
print m.group(1)
print m.group(2)

