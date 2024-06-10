import os
from collections import Counter

with open("log") as fi:
    lines = [l.strip() for l in fi.readlines()]
    print(Counter(lines))
