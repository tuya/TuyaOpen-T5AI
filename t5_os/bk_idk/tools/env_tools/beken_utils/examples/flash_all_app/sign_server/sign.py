#!/usr/bin/env python3

import logging
import subprocess
import os
import shutil

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

def sign_server_process(tool_path, ota_type, app_version, app_security_counter, ota_version, ota_security_counter, privkey, pubkey, replace_key_en, new_privkey, new_pubkey, app_slot_size, ota_slot_size, bl1_secureboot_en, bl2_load_addr, debug_opt=''):
    debug_opt=''
    if bl1_secureboot_en:
        cmd = f'{tool_path}/main.py sign bl1_sign --action_type sign --key_type ec256 --privkey_pem_file {privkey} --pubkey_pem_file {pubkey} --bin_file bl2.bin --load_addr {bl2_load_addr} --outfile primary_manifest.bin'
        run_cmd(cmd)
        save_cmd("Sign and create primary_manifest.bin", cmd)

    if replace_key_en:
        cmd = f'{tool_path}/scripts/../tools/mcuboot_tools/imgtool.py sign -k {new_privkey} --public-key-format full --max-align 8 --align 1 --version {app_version} --security-counter {app_security_counter} --pad-header --header-size 0x1000 --slot-size {app_slot_size} --pad --boot-record SPE --endian little --encrypt-keylen 128 primary_all.bin primary_all_signed.bin  --action_type sign  --pubkeyfile {pubkey} {debug_opt}'
        run_cmd(cmd)
        save_cmd("Sign with new key and create primary_all_signed.bin from primary_all.bin", cmd)
    else:
        cmd = f'{tool_path}/scripts/../tools/mcuboot_tools/imgtool.py sign -k {privkey} --public-key-format full --max-align 8 --align 1 --version {app_version} --security-counter {app_security_counter} --pad-header --header-size 0x1000 --slot-size {app_slot_size} --pad --boot-record SPE --endian little --encrypt-keylen 128 primary_all.bin primary_all_signed.bin  {debug_opt}'
        run_cmd(cmd)
        save_cmd("Sign and create primary_all_signed.bin from primary_all.bin", cmd)

    if ota_type == "XIP":
        cmd = f'{tool_path}/scripts/../tools/mcuboot_tools/imgtool.py sign -k {privkey} --public-key-format full --max-align 8 --align 1 --version {ota_version} --security-counter {ota_security_counter} --custom-tlv 0xa0 0x{pubkey} --pad-header --header-size 0x1000 --slot-size {app_slot_size} --pad --boot-record SPE --endian little --encrypt-keylen 128 primary_all.bin ota_signed.bin --action_type sign --pubkeyfile {pubkey} {debug_opt}'
        run_cmd(cmd)
        save_cmd("Sign and create XIP ota_signed.bin from ota.bin", cmd)
    else:
        cmd = f'{tool_path}/main.py pack compress --infile primary_all_signed.bin --outfile compress.bin {debug_opt}'
        run_cmd(cmd)
        save_cmd("Sign and create Overwrite ota_signed.bin from ota.bin", cmd)

        cmd = f'{tool_path}/scripts/../tools/mcuboot_tools/imgtool.py sign -k {privkey} --public-key-format full --max-align 8 --align 1 --version {ota_version} --security-counter {ota_security_counter} --custom-tlv 0xa0 0x{pubkey} --pad-header --header-size 0x1000 --slot-size {ota_slot_size} --pad --boot-record SPE --endian little --encrypt-keylen 128 compress.bin ota_signed.bin --action_type sign  --pubkeyfile {pubkey} {debug_opt}'
        run_cmd(cmd)
        save_cmd("Sign and create Overwrite ota_signed.bin from ota.bin", cmd)
