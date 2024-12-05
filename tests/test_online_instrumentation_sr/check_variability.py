#!/bin/env python

import numpy as np
import argparse
import sys
import significantdigits as sd


def read_file(filename):
    # use np.loadtxt to read the file where each lines is float numbers in hex format separated by a space
    with open(filename, "r") as file:
        raw = [
            [float.fromhex(f.strip()) for f in line.split()]
            for line in file.readlines()
            if not line.startswith("#")
        ]
    x = np.array(raw, dtype=np.float64)

    return x


def compute_sig(x, verbose):
    mean = np.mean(x, axis=0)
    std = np.std(x, axis=0)
    sig = sd.significant_digits(x, reference=mean, basis=10)
    sig_min = np.min(sig)
    mean_min = np.min(np.abs(mean))
    if verbose:
        print(f"Mean: {mean}")
        print(f"Min mean: ", mean_min.hex())
        print(f"Std: {std}")
        print(f"Signficant digits: {sig}")
        print(f"Min significant digits: {sig_min}")
    return sig_min, mean_min


def deduce_type(filename, fp_type):
    if "float" in filename or fp_type == "float":
        return "float"
    if "double" in filename or fp_type == "double":
        return "double"
    raise Exception("Invalid floating-point type" + fp_type)


def parse_args():
    parser = argparse.ArgumentParser()
    parser.add_argument("input", type=str)
    parser.add_argument("--type", help="floating-point type of inputs")
    parser.add_argument("--verbose", action="store_true", help="verbose mode")
    return parser.parse_args()


def main():
    args = parse_args()

    try:
        fp_type = deduce_type(args.input, args.type)
        x = read_file(args.input)
        sig_min, mean_min = compute_sig(x, args.verbose)
    except Exception as e:
        print(f"Error with {args.input}")
        print(e)
        sys.exit(1)

    tol = 0
    if fp_type == "float":
        tol = 6
    elif fp_type == "double":
        tol = 14

    if args.verbose:
        print(f"Tolerance: {tol}")

    if sig_min < tol:
        print(
            f"Error with {args.input}: minimal number of significant digits {sig_min:.2f} below threshold ({tol:.2f})"
        )
        sys.exit(1)

    if mean_min == 0:
        print(f"Error with {args.input}: one element is zero")
        sys.exit(1)


if "__main__" == __name__:

    main()
