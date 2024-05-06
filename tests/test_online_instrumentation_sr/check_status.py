import numpy as np
import sys
with open(sys.argv[1]) as fi:
    x = [float.fromhex(l.strip()) for l in fi]
    print(int(len(set(x)) == 1))
