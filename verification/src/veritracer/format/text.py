import os
from veritracer.format import format 
from veritracer.format import stats 

# return size in bytes associated to the given format
def parse_format(fmt):
    if fmt.find('32') != -1:
        return 4
    elif fmt.find('64') != -1:
        return 8
    else:
        print("Error: unknown format {format}".format(format=fmt))
        exit(1)

# Args order fmt,time,hash,ptr,val
def parse_raw_line(line):

    line_split = line.split()

    size  = parse_format(line_split[0])
    ptr   = line_split[3]
    time  = int(line_split[1])
    hashv = int(line_split[2])        
    val = stats.hexa_to_fp(line_split[4])

    value = vtr_fmt.ValuesLine(format=size,
                               time=time,
                               address=ptr,
                               hash=hashv,
                               value=val) 
    return value

def parse_file(filename, offset):
    try:
        fi = open(filename, 'r+b')
        i = 0
        while(i<offset):
            fi.readline()
            i += 1

    except IOError as e:
        print("Could not open {filename} -> {err}".format(filename=filename, err=str(e)))
        return

    if os.path.getsize(filename) == 0:
        print("Error: {format} is empty".format(filename=filename))
        exit(1)
    values_list = [parse_raw_line(line) for line in fi]
    return values_list
