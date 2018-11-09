#!/usr/bin/env python

import sys
import argparse
import os
import csv
from collections import namedtuple
import shutil
import multiprocessing as mp
from functools import partial

import veritracer_backtrace as vtr_backtrace
import veritracer_math as vtr_math
import veritracer_format.binary as fmtbinary
import veritracer_format.text as fmttext
import veritracer_format.veritracer_format as vtr_fmt

csv_header = ["hash","type","time","max","min","median","mean","std","significant_digit_number"]

local_offset = 0

StatsLine = namedtuple('StatsLine',['hash','type','time',
                                    'max','min','mean',
                                    'median','std','significant_digit_number'])

default_traces_path=".vtrace"

Ko = 1024
Mo = Ko*Ko
Go = Mo*Mo

mem_units = ['K', 'M', 'G']
unit_in_bytes = {'K':Ko,'M':Mo,'G':Go}

format_file_binary = "binary"
format_file_text = "text"
formats_file = [format_file_binary, format_file_text]
format_file = None

data_checking=False

def init_module(subparsers, veritracer_plugins):
    veritracer_plugins["analyze"] = run
    analyze_parser = subparsers.add_parser("analyze", help="Gathering values from several veritracer executions")
    analyze_parser.add_argument('--filename', type=str, default="veritracer.dat", metavar='',
                                 help="filename of the trace to gather")
    analyze_parser.add_argument('-o','--output', type=str, default="veritracer", metavar='',
                                 help='output filename')
    analyze_parser.add_argument('--read-per-bytes', type=str, default='0',
                                 help='read per byte packet of N, can use Ko, Mo, Go extensions')
    analyze_parser.add_argument('--prefix-dir', type=str, default=default_traces_path, metavar='',
                                 help='prefix of the directory to analyze (default {default})'.format(
                                     default=default_traces_path))
    analyze_parser.add_argument('--backtrace-filename', type=str, default="backtrace.dat", metavar='',
                                 help='filename of the backtrace to use')
    analyze_parser.add_argument('--verbose', action="store_true",
                                 help="verbose mode")
    analyze_parser.add_argument('--format', action='store', choices=formats_file, default='binary',
                                 help='veritracer.dat encoding format')
    analyze_parser.add_argument('--data-checking', action='store_true',
                                help='enables asserts')
    
def parse_file(filename, offset):
    if format_file == "binary":
        return fmtbinary.parse_file(filename, offset)
    elif format_file == "text":
        return fmttext.parse_file(filename, offset)
    else:
        assert(0)
        
def parse_exp(exp):
    format = map(lambda valueLine : valueLine.format, exp)
    time = map(lambda valueLine : valueLine.time, exp)
    address = map(lambda valueLine : valueLine.address, exp)
    hashv = map(lambda valueLine : valueLine.hash, exp)
    value = map(lambda valueLine : valueLine.value, exp)

    valueline = vtr_fmt.ValuesLine(format=format[0],
                                   time=time[0],
                                   address=address[0],
                                   hash=hashv[0],
                                   value=value)

    return valueline

def parse_directory(offset, file_):
    return parse_file(file_, offset)

def currying(f,x):
    return f(*x)

def get_files():    
    filename = args.filename
    list_dir = deque(os.listdir('.'))
    list_directories = filter(os.path.isdir, list_dir)
    list_files = map(lambda d: os.path.join(d,filename), list_directories)
    return filter(os.path.isfile, list_files)

# # Each value is a tuple 
# def compute_stats(values_list):

#     sizeof_value = values_list.format
#     time         = values_list.time
#     ptr          = values_list.address
#     hashv        = values_list.hash
#     list_FP      = values_list.value
#     mean_   = vtr_math.mean(list_FP)
#     std_    = vtr_math.std(mean_, list_FP)
#     median_ = vtr_math.median(list_FP)
#     max_    = max(list_FP)
#     min_    = min(list_FP)    
#     sdn_    = vtr_math.sdn(mean_, std_, sizeof_value)

#     stats = StatsLine(hash=hashv,
#                       type=sizeof_value,
#                       time=time,   
#                       max=max_,
#                       min=min_,
#                       mean=mean_,
#                       median=median_,
#                       std=std_,
#                       significant_digit_number=sdn_)

#     return stats


# Each value is a tuple 
def compute_stats(valuesLine_list):

    # We take the time from one sample, maybe take the mean ?
    
    time         = valuesLine_list[0].time
    address      = valuesLine_list[0].address
    sizeof_value = valuesLine_list[0].format
    hash_value   = valuesLine_list[0].hash
    list_values  = map(lambda valueLine : valueLine.value, valuesLine_list)

    if data_checking:
        hash_value   = set(map(lambda valueLine : valueLine.hash, valuesLine_list))
        sizeof_value = set(map(lambda valueLine : valueLine.format, valuesLine_list))
        assert(len(sizeof_value) == 1)
        assert(len(hash_value) == 1)
    
    mean_   = vtr_math.mean(list_values)
    std_    = vtr_math.std(mean_, list_values)
    median_ = vtr_math.median(list_values)
    max_    = max(list_values)
    min_    = min(list_values)    
    sdn_    = vtr_math.sdn(mean_, std_, sizeof_value)

    stats = StatsLine(hash=hash_value,
                      type=sizeof_value,
                      time=time,   
                      max=max_,
                      min=min_,
                      mean=mean_,
                      median=median_,
                      std=std_,
                      significant_digit_number=sdn_)

    return stats

def parse_values(exp):    
    return compute_stats(exp)

def open_csv(filename):
    output = filename
    fo = open(output, 'wb')
    csv_writer = csv.DictWriter(fo, fieldnames=csv_header)
    csv_writer.writeheader()
    return csv_writer

def write_csv(filename, list_errors):
    csv_writer = open_csv(filename)
    for row in list_errors:
        csv_writer.writerow(row._asdict())
        
def get_read_per_bytes_size(args):
    read_bytes = args.read_per_bytes
    if read_bytes == "0":
        return 0
    elif read_bytes.isdigit():
        return int(read_bytes)
    else:
        for unit in mem_units:
            if read_bytes.find(unit) != -1:
                B = read_bytes.split(unit)[0]
                if B.isdigit():
                    return int(B) * unit_in_bytes[unit]
                else:
                    print "Unknown size: {size}".format(read_bytes)
                    exit(1)
    

def analyze_files_list(jobs_pool, files_list, offset):
    parse_directory_currying = partial(parse_directory, offset)
    exp_list_transposed = jobs_pool.map(parse_directory_currying, files_list)
    exp_list = zip(*exp_list_transposed)
    values_list = jobs_pool.map(parse_values, exp_list)
    return values_list    
        
def concat_partial_files(output_filename, nb_slices):
    fo = open(output_filename, "w")
    for slc in range(nb_slices):
        output_filename_slice = "{output}.{slc}".format(output=output_filename,slc=slc)
        fi = open(output_filename_slice, "r")
        shutil.copyfileobj(fi, fo)
        fi.close()
        os.remove(output_filename_slice)

def set_format_file(args):
    global format_file
    if args.format == "binary":
        format_file = format_file_binary
    elif args.format == "text":
        format_file = format_file_text
    else:
        print "Unknown format file: {fmt}".format(fmt=args.format)
        exit(1)

def set_data_checking(args):
    if args.data_checking:
        data_checking=True
        
def run(args):

    set_format_file(args)
    set_data_checking(args)
    
    if args.prefix_dir and os.path.isdir(args.prefix_dir):
        os.chdir(args.prefix_dir)
    else:
        print "Unknown directory {dir}".format(dir=args.prefix_dir)
        return False

    per_bytes = get_read_per_bytes_size(args)

    # Dict which maps bt_name to associated directories
    try:
        dict_bt_files = vtr_backtrace.partition_samples(args)
    except vtr_backtrace.BacktraceFileNoExist as bt_error:
        print "Error: backtrace file {bt} does not exist".format(bt=bt_error.message)
        return False
    except vtr_backtrace.AllEmptyFile as empty_error:
        print "Error: all {filename} files are empty".format(filename=empty_error.message)
        return False
    
    jobs_pool = mp.Pool()
    
    for bt_name, dir_list in dict_bt_files.iteritems():

        if args.verbose:
            print bt_name, dir_list
                
        files_list = map(lambda d : d + os.sep + args.filename, dir_list)
        files_size = set(map(os.path.getsize, files_list))
        
        if format_file == format_file_binary:            
            assert(len(files_size) == 1)
        # for text format we need to count the number of line of
        # each files which could be time onerous, so we 
        
        filesize = files_size.pop()
        if format_file == format_file_text:
            # If we are in text mode, we divide by the average size in bytes of a line
            filesize /= 80
            
        output_filename = "{output}.{bt}".format(output=args.output, bt=bt_name)

        nb_slices = 1 if per_bytes == 0 else max(1,filesize/per_bytes)
        offset = 0 if nb_slices < 2 else filesize/per_bytes

        for slc in range(nb_slices):
            values_list = analyze_files_list(jobs_pool, files_list, offset)            
            output_filename_slice = "{output}.{slc}".format(output=output_filename,slc=slc)
            write_csv(output_filename_slice, values_list)

        concat_partial_files(output_filename, nb_slices)
        
    return True
