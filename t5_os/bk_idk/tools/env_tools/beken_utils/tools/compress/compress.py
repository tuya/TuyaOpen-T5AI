#!/usr/bin/env python3
import logging
import math
import struct
import sys
import argparse
import os
import subprocess

def run_cmd(cmd):
    p = subprocess.Popen(cmd, shell=True)
    ret = p.wait()
    if (ret):
        logging.error(f'failed to run "{cmd}"')
        exit(1)

def get_script_dir():
    script_dir = os.path.abspath(__file__)
    script_dir = os.path.dirname(script_dir)
    return script_dir

def is_win():
    return os.name == 'nt'

def is_pi():
    if os.path.exists('/proc/cpuinfo'):
        with open('/proc/cpuinfo', 'r') as f:
            cpuinfo = f.read()
            if 'Raspberry Pi' in cpuinfo:
                return True
        return False
    elif os.path.exists('/etc/os-release'):
        with open('/etc/os-release', 'r') as f:
            os_info = f.read().lower()
            if 'raspbian' in os_info or 'raspberry' in os_info:
                return True
            else:
                return False
    else:
        return False

def is_centos():
    return True

def get_lzma_tool():
    script_dir = get_script_dir()
    if os.name == 'nt':
        return f'{script_dir}\\lzma.exe'

    if is_pi():
        return f'{script_dir}/lzma_pi'

    return f'{script_dir}/lzma'

COMPRESS_BLOCK_SZ = 0x10000

def compress_bin(infile, outfile):
    compress_size_list = []
    compress_temp_in = 'temp_before_compress'
    compress_temp_out = 'temp_after_compress'
    file_size = os.path.getsize(infile)
    with open(infile,'rb') as src,open(outfile,'w+b') as dst:
        offset = 2 * math.floor(file_size/COMPRESS_BLOCK_SZ) + 4 # 2 uint16_t for after and before block size
        logging.debug(f'block num = {offset //2 - 1}')
        dst.seek(offset)
        sum = 0
        src.seek(0)
        idx = 0
        while True:
            file_in = open(compress_temp_in,"wb+")
            uncompress_block_size = bytes()
            chunk = bytes() # clear chunk
            chunk = src.read(COMPRESS_BLOCK_SZ)
            if(len(chunk) != COMPRESS_BLOCK_SZ):
                uncompress_block_size = struct.pack("H",len(chunk))
            if not chunk:
                break
            file_in.seek(0)
            file_in.write(chunk)
            file_in.close()
            
            script_dir = get_script_dir()
            compress_tool = get_lzma_tool()
            cmd =f'{compress_tool} e {compress_temp_in} {compress_temp_out}'
            run_cmd(cmd)
            file_out = open(compress_temp_out,"rb")
            chunk = bytes() # clear chunk
            file_out.seek(0)
            chunk = file_out.read()
            compress_chunk_size = len(chunk)
            logging.debug(f'block after size:{compress_chunk_size}')
            compress_chunk_size = struct.pack("H",compress_chunk_size)
            compress_size = compress_chunk_size + uncompress_block_size
            compress_size_list.append(compress_size)
            dst.write(chunk)
            sum += len(chunk)
            file_out.close()
        offset = 0
        dst.seek(offset)
        for num in compress_size_list:
            dst.write(num)
        os.remove(compress_temp_in)
        os.remove(compress_temp_out)

def main():
    parse = argparse.ArgumentParser(description="Beken compress tool")
    parse.add_argument('--infile', type=str, required=True, help='Specify input file')
    parse.add_argument('--outfile', type=str, required=False, help='Specify output file')

    args = parse.parse_args()
    if os.path.exists(args.infile) == False:
        logging.error(f'{infile} not exist')
        exit(1)

    if args.outfile == None:
        args.outfile = f'{args.infile}.compressed'

    compress_bin(args.infile, args.outfile)

if __name__ == '__main__':
    logging.basicConfig()
    logging.getLogger().setLevel(logging.DEBUG)
    stream_handler = logging.StreamHandler(sys.stdout)
    stream_handler.setLevel(logging.DEBUG)
    main()
