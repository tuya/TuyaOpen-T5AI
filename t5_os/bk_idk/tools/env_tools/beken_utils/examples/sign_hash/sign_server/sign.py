#!/usr/bin/env python3

import logging
import subprocess
import os
import shutil
import json

def save_cmd(description, cmd):
    cmd_list_file = 'sign_cmd_list.txt'
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

def get_app_sig(file_name):
    with open(file_name, 'r') as f:
        d = json.load(f)
        return d['signature']

def get_bl2_sig(file_name):
    with open(file_name, 'r') as f:
        d = json.load(f)
        return [d['bl1_sig_r'], d['bl1_sig_s']]

def sign_server_sign_app_bin_hash(tool_path, bl2_bin_hash, bl2_b_bin_hash, app_bin_hash):
    logging.debug(f'')
    logging.debug(f'--------------------On sign server: sign hash of binaries--------------------')
    cmd = f'{tool_path}/main.py steps sign_app_bin_hash --bl2_bin_hash {bl2_bin_hash} --bl2_b_bin_hash {bl2_b_bin_hash} --app_bin_hash {app_bin_hash} --debug'
    run_cmd(cmd)
    save_cmd("Sign bin hash", cmd)

    sig_list = get_bl2_sig('primary_manifest_sig.json')
    bl2_b_sig = get_bl2_sig('secondary_manifest_sig.json')
    sig_list.extend(bl2_b_sig)
    app_sig = get_app_sig('app_sig.json')
    sig_list.append(app_sig)
    return sig_list

def sign_server_sign_ota_bin_hash(tool_path, ota_bin_hash):
    logging.debug(f'')
    logging.debug(f'--------------------On sign server: sign hash of OTA binaries--------------------')
    cmd = f'{tool_path}/main.py steps sign_ota_bin_hash --ota_bin_hash {ota_bin_hash} --debug'
    run_cmd(cmd)
    save_cmd("Sign OTA bin hash", cmd)

    ota_sig = get_app_sig('ota_sig.json')
    return ota_sig

