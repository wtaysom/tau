file = open('pepper.txt')
text = file.read()

import string
paragraphs = string.split(text, '\n\n')

import re
matchstr = re.compile(
    r"""\b(red|green)
        (\s+
         pepper
         (?!cord)
         (?=.*salad))""",
      re.IGNORECASE |
      re.DOTALL |
      re.VERBOSE)
for paragraph in paragraphs:
    fixed_paragraph = matchstr.sub(r'bell\2', paragraph)
    print fixed_paragraph+'\n'
