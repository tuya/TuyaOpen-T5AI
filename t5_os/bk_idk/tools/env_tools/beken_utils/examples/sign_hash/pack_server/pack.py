#!/usr/bin/env python3

import logging
import subprocess
import os
import shutil
import json

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

def run_cmd(cmd):
    p = subprocess.Popen(cmd, shell=True)
    ret = p.wait()
    if (ret):
        logging.error(f'failed to run "{cmd}"')
        exit(1)

def pack_server_pack(tool_path):
    logging.debug(f'')
    logging.debug(f'--------------------On pack server: pack all--------------------')
    cmd = f'{tool_path}/main.py steps pack_csv --debug'
    run_cmd(cmd)

def get_hash(file_name):
    with open(file_name, 'r') as f:
        d = json.load(f)
        return d['hash']

def pack_server_get_app_bin_hash(tool_path):
    logging.debug(f'')
    logging.debug(f'--------------------On pack server: calculate hash of binaries--------------------')
    cmd = f'{tool_path}/main.py steps get_app_bin_hash --debug'
    run_cmd(cmd)
    save_cmd("Get APP bin HASH", cmd)

    bl2_bin_hash = get_hash('primary_manifest_hash.json')
    bl2_b_bin_hash = get_hash('secondary_manifest_hash.json')
    app_bin_hash = get_hash('app_hash.json')
    return [bl2_bin_hash, bl2_b_bin_hash, app_bin_hash]

def pack_server_sign_from_app_sig(tool_path, bl2_sig_r, bl2_sig_s, bl2_b_sig_r, bl2_b_sig_s, app_sig):
    logging.debug(f'')
    logging.debug(f'--------------------On pack server: create signed bin from signature--------------------')
    cmd = f'{tool_path}/main.py steps sign_from_app_sig --bl2_sig_r {bl2_sig_r} --bl2_sig_s {bl2_sig_s} --bl2_b_sig_r {bl2_b_sig_r} --bl2_b_sig_s {bl2_b_sig_s} --app_sig {app_sig} --debug'
    run_cmd(cmd)
    save_cmd("", cmd)

def pack_server_get_ota_bin_hash(tool_path):
    logging.debug(f'')
    logging.debug(f'--------------------On pack server: calculate hash of OTA binaries--------------------')
    cmd = f'{tool_path}/main.py steps get_ota_bin_hash --debug'
    run_cmd(cmd)
    save_cmd("Get OTA bin HASH", cmd)

    ota_bin_hash = get_hash('ota_hash.json')
    return ota_bin_hash

def pack_server_sign_from_ota_sig(tool_path, ota_bin_sig):
    logging.debug(f'')
    logging.debug(f'--------------------On pack server: create OTA signed bin from signature--------------------')
    cmd = f'{tool_path}/main.py steps sign_from_ota_sig --ota_bin_sig {ota_bin_sig} --debug'
    run_cmd(cmd)
    save_cmd("", cmd)
