#!/usr/bin/env python3

import logging
import subprocess
import os
import shutil

def run_cmd(cmd):
    p = subprocess.Popen(cmd, shell=True)
    ret = p.wait()
    if (ret):
        logging.error(f'failed to run "{cmd}"')
        exit(1)

def pack_server_process(tool_path, bl1_secureboot_en, flash_aes_type, flash_aes_key, ota_type, ota_security_counter, boot_ota, bl2_version, pubkey_pem_file, debug_opt=''):
    cmd = f'{tool_path}/main.py gen otp_efuse --flash_aes_type {flash_aes_type} --flash_aes_key flash_aes_key --pubkey_pem_file {pubkey_pem_file} --outfile otp_efuse_config.json {debug_opt}'
    run_cmd(cmd)

    if bl1_secureboot_en:
        bl1_secureboot_en_opt = '--bl1_secureboot_en'
    else: 
        bl1_secureboot_en_opt = ''

    if boot_ota:
        boot_ota_opt = '--boot_ota'
    else:
        boot_ota_opt = ''

    cmd = f'{tool_path}/main.py steps pack {bl1_secureboot_en_opt} --flash_aes_type {flash_aes_type} --flash_aes_key {flash_aes_key} --ota_type {ota_type} --ota_security_counter {ota_security_counter} {boot_ota_opt} --bl2_version {bl2_version} --pubkey_pem_file {pubkey_pem_file} {debug_opt}'
    run_cmd(cmd)
