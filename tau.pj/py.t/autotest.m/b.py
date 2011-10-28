file = open('memcpu_test.txt')
text = file.read()

import re
result = re.match(r"""L1\ cache\ data:""", text)

print result
