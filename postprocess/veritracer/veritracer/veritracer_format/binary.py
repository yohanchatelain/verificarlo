#!/usr/bin/env python

import os
import mmap
import struct
from collections import deque

import veritracer_format as vtr_fmt

sizeof_veritracer_line_struct = 36
sizeof_binary32 = 4
sizeof_binary64 = 8

# Args order fmt,time,hash,ptr,val
def parse_raw_line(line):
    # print line
    rawvalue = vtr_fmt.ValuesLine(format=line[0],
                                  time=line[1],
                                  hash=line[3],
                                  address=line[2],
                                  value=line[4])
    
    size  = rawvalue.format
    ptr   = "0x0" if rawvalue.address == "(nil)" else rawvalue.address
    time  = int(rawvalue.time)
    hashv = int(rawvalue.hash)        
    val   = rawvalue.value
    value = vtr_fmt.ValuesLine(format=size,
                               time=time,
                               address=ptr,
                               hash=hashv,
                               value=val)
    
    return value


# https://docs.python.org/2/library/struct.html#format-characters
# struct Format Characters
# I : unsigned integer, 4 bytes
# Q : long long unsigned integer, 8 bytes
# P : void*, 8 bytes
# f : float, 4 bytes
# d : double, 8 bytes
def parse_file(filename, offset):

    try:
        print filename
        fi = open(filename, 'rb')
        fi.seek(offset)
    except IOError as e:
        print "Could not open {filename} -> {err}".format(filename=filename, err=str(e))
        return

    if os.path.getsize(filename) == 0:
        print "Error: {filename} is empty".format(filename=filename)
        exit(1)
    
    values_list = deque()
    append = values_list.append

    bytes_read = fi.read(sizeof_veritracer_line_struct)

    while bytes_read != '':
        values_read = struct.unpack('=IQQQxxxxxxxx', bytes_read)
        sizeof_data = values_read[0]
        if sizeof_data == sizeof_binary32:
            values_read = struct.unpack('=IQQQfxxxx', bytes_read)
        elif sizeof_data == sizeof_binary64:
            values_read = struct.unpack('=IQQQd', bytes_read)
        else:
            print "Unknown size : {size}o".format(size=sizeof_data)
            exit(1)
        
        values_line = parse_raw_line(values_read)
        append(values_line)

        bytes_read = fi.read(sizeof_veritracer_line_struct)
        
    return values_list
