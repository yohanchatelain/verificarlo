#!/usr/bin/env python3

import os
import veritracer.format as vtr_fmt
import veritracer.stats as vtr_math

# Return size in bytes associated to the given format


def parse_format(fmt):
    if fmt.find('32') != -1:
        return 4
    elif fmt.find('64') != -1:
        return 8
    else:
        print(f"Error: unknown format {fmt}")
        exit(1)

# Args order fmt,time,hash,ptr,val


def parse_raw_line(line):

    line_split = list(map(lambda x: x.decode(), line.split()))

    size = parse_format(line_split[0])
    ptr = line_split[3]
    time = int(line_split[1])
    hashv = int(line_split[2])
    val = vtr_math.hexa_to_fp(line_split[4])

    value = vtr_fmt.ValuesLine(format=size,
                               time=time,
                               address=ptr,
                               hash=hashv,
                               value=val)
    return value


def parse_file(filename, offset):
    try:
        fi = open(filename, 'r+b')
        fi.seek(offset)
    except IOError as e:
        print(f"Could not open {filename} -> {str(e)}")
        return

    if os.path.getsize(filename) == 0:
        print(f"Error: {filename} is empty")
        exit(1)

    values_list = [parse_raw_line(line) for line in fi]
    return values_list
