#!/usr/bin/env python3

import logging

def save_cmd(description, cmd):
    cmd_list_file = 'pack_cmd_list.txt'
    with open(cmd_list_file, 'a') as f:
        f.write('\r\n')
        f.write(description)
        f.write('\r\n')
        split_line = len(description)*'-'
        f.write(split_line)
        f.write('\r\n')
        f.write(cmd)
        f.write('\r\n')

def save_bk_loader_cmd(partition_name, bin_name, offset):
    cmd = f'bk_loader.exe download --portinfo 18 --baudrate 1500000 --infile {bin_name} --startaddress {hex(offset)} --reboot 0'
    save_cmd(f'Flashing {partition_name}:{bin_name} to flash offset {hex(offset)}', cmd)
