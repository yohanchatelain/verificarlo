#!/usr/bin/env python3

import numpy as np
import math

SDN_MAX = {4: 24, 8: 52}


def log_base_convert(x, from_base, to_base):
    return x * math.log(from_base, to_base)


def std(x):
    return np.std(x)


def median(x):
    return np.median(x)


def mean(list_FP):
    return np.mean(list_FP)

# Compute the number of significant digits following definition given by Parker
# s = - log_base | sigma / mu |, where base is the calculating basis


def sdn(mean, std, size_bytes, base=10):
    if std != 0.0:
        if mean != 0.0:
            return -math.log(abs(std/mean), base)
        else:
            return 0
    else:  # std == 0.0
        return log_base_convert(SDN_MAX[size_bytes], 2, base)

# Translate from C99 hexa representation to FP
# 0x+/-1.mp+e, m mantissa and e exponent


def hexa_to_fp(hexafp):
    return float.fromhex(hexafp)
