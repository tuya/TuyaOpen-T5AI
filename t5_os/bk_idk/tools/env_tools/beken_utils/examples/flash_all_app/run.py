#!/usr/bin/env python3

import argparse
import logging
import subprocess
import os
import shutil
import sys

from build_server.build import *
from sign_server.sign import *
from pack_server.pack import *

def go_back_dir(levels):
    cwd = os.getcwd()
    for i in range(levels):
        cwd = os.path.dirname(cwd)
    return cwd

def copy_file(src, dst):
    if os.path.exists(src):
        shutil.copy(src, dst)

def copy_files(src, dst):
    files = os.listdir(src)
    for f in files:
        shutil.copy(f'{src}/{f}', dst)

def rm_file(f):
    if os.path.exists(f):
        os.remove(f)

def rm_dir(d):
    if os.path.exists(d):
        for f in os.listdir(d):
            f = os.path.join(d, f)
            if os.path.isfile(f):
                os.remove(f)
            elif os.path.isdir(f):
                os.rmtree(f)

def run_cmd(cmd):
    p = subprocess.Popen(cmd, shell=True)
    ret = p.wait()
    if (ret):
        logging.error(f'failed to run "{cmd}"')
        exit(1)

def set_debug(debug):
    if debug:
        logging.basicConfig()
        logging.getLogger().setLevel(logging.DEBUG)
        stream_handler = logging.StreamHandler(sys.stdout)
        stream_handler.setLevel(logging.DEBUG)
    else:
        logging.basicConfig()
        logging.getLogger().setLevel(logging.INFO)
        stream_handler = logging.StreamHandler(sys.stdout)
        stream_handler.setLevel(logging.INFO)

def main():
    parse = argparse.ArgumentParser(description="Sign, encrypt and pack all-app.bin")
    parse.add_argument('--flash_aes_type', choices=['NONE', 'FIXED', 'RANDOM'], required=True, help='Specify flash AES type')
    parse.add_argument('--flash_aes_key', type=str, help='Specify flash AES key')
    parse.add_argument('--bl2_version', type=str, required=True, help='Specify bl2.bin version')
    parse.add_argument('--app_version', type=str, required=True, help='Specify app.bin version')
    parse.add_argument('--app_security_counter', type=int, required=True, help='Specify app.bin security counter')
    parse.add_argument('--ota_version', type=str, required=True, help='Specify ota.bin version')
    parse.add_argument('--ota_security_counter', type=str, required=True, help='Specify ota.bin security counter')
    parse.add_argument('--ota_type', choices=['XIP', 'OVERWRITE'], required=True, help='Specify OTA strategy')
    parse.add_argument('--privkey', type=str, required=False, help='Specify BL2 private key PEM file')
    parse.add_argument('--pubkey', type=str, required=True, help='Specify BL2 public key PEM file')
    parse.add_argument('--replace_key_en', action='store_true', help='whether to replace key')
    parse.add_argument('--new_privkey', type=str, required=False, help='new Specify BL2 private key PEM file')
    parse.add_argument('--new_pubkey', type=str, required=False, help='new Specify BL2 public key PEM file')
    parse.add_argument('--app_slot_size', type=int, required=True, help='Specify virtual APP slot size')
    parse.add_argument('--ota_slot_size', type=int, help='Specify virtual OTA slot size (OVERWRITE only)')
    parse.add_argument('--project', type=str, help='Specify the project name')
    parse.add_argument('--bl1_secureboot_en', action='store_true', help='whether enable secureboot')
    parse.add_argument('--bl2_load_addr', type=str, help='Bootloader(BL2) load address')
    parse.add_argument('--boot_ota', action='store_true', help='Indicate whether support bootloader OTA')
    parse.add_argument('--build', action='store_true', help='Indicate whether rebuild the project')
    parse.add_argument('--clean', action='store_true', help='Clean all temp files')
    parse.add_argument('--debug', action='store_true', help='Enable debug')

    args = parse.parse_args()
    cwd = os.getcwd()
    tool_path = go_back_dir(2)
    build_path = os.path.join(cwd, 'build_server')
    sign_path = os.path.join(cwd, 'sign_server')
    pack_path = os.path.join(cwd, 'pack_server')

    if args.flash_aes_type == 'FIXED':
        if args.flash_aes_key == None:
            logging.error(f'option "--flash_aes_key" is required when option "--flash_aes_type" is FIXED')
            exit(1)

    if args.bl1_secureboot_en:
        if args.bl2_load_addr == None:
            logging.error(f'option "--bl2_load_addr" is required when "--bl1_secureboot_en" is specified')
            exit(1)

    if args.ota_type == 'OVERWRITE':
        if args.ota_slot_size == None:
            logging.error(f'when ota_type is "OVERWRITE" then "--ota_slot_size" is required')
            exit(1)

    if args.build:
        if args.project == None:
            logging.error(f'option "--project" is required when "--build" is specified')
            exit(1)

    set_debug(args.debug)
    if args.debug:
        debug_opt = '--debug'
    else:
        debug_opt = ''
    
    if args.build:
        idk_path = go_back_dir(5)
        os.chdir(idk_path)
        build_server_process(args.project)
        project_base = os.path.basename(args.project)
        copy_file(f'{idk_path}/build/{project_base}/bk7236/_build/primary_all.bin', f'{sign_path}/')

        if args.bl1_secureboot_en:
            copy_file(f'{idk_path}/build/{project_base}/bk7236/_build/bl2.bin', f'{sign_path}/')
            copy_file(f'{idk_path}/build/{project_base}/bk7236/_build/provision.bin', f'{pack_path}/immutable')
            copy_file(f'{idk_path}/build/{project_base}/bk7236/_build/primary_manifest.json', f'{pack_path}/immutable')

        logging.debug(f'Copy the immutable bin or configurations to pack server')
        copy_file(f'{idk_path}/build/{project_base}/bk7236/_build/bl2.bin', f'{pack_path}/immutable')
        copy_file(f'{idk_path}/build/{project_base}/bk7236/_build/nvs.csv', f'{pack_path}/immutable')
        copy_file(f'{idk_path}/build/{project_base}/bk7236/_build/pack.json', f'{pack_path}/immutable')
        copy_file(f'{idk_path}/build/{project_base}/bk7236/_build/partitions.csv', f'{pack_path}/immutable')
        copy_file(f'{idk_path}/build/{project_base}/bk7236/_build/root_ec256_pubkey.pem', f'{pack_path}/immutable')
 
    
    os.chdir(sign_path)
    sign_server_process(tool_path, args.ota_type, args.app_version, args.app_security_counter, args.ota_version, args.ota_security_counter, args.privkey, args.pubkey, args.app_slot_size, args.ota_slot_size, args.bl1_secureboot_en, args.bl2_load_addr, debug_opt)

    if os.path.exists(f'{pack_path}/_tmp'):
        shutil.rmtree(f'{pack_path}/_tmp')
    os.makedirs(f'{pack_path}/_tmp', exist_ok=True)
    logging.debug(f'Copy signed bin to pack dir for packing');
    copy_file(f'{sign_path}/primary_all_signed.bin', f'{pack_path}/_tmp')
    copy_file(f'{sign_path}/ota_signed.bin', f'{pack_path}/_tmp')
    shutil.move(f'{sign_path}/pack_cmd_list.txt', f'{pack_path}/_tmp')
    copy_files(f'{pack_path}/immutable', f'{pack_path}/_tmp')

    if args.bl1_secureboot_en:
        copy_file(f'{sign_path}/primary_manifest.bin', f'{pack_path}/_tmp')

    os.chdir(f'{pack_path}/_tmp')
    pack_server_process(tool_path, args.bl1_secureboot_en, args.flash_aes_type, args.flash_aes_key, args.ota_type, args.ota_security_counter, args.boot_ota, args.bl2_version, args.pubkey, debug_opt)

    os.chdir(cwd)
    if os.path.exists(f'{cwd}/install'):
        shutil.rmtree(f'{cwd}/install')
    os.makedirs(f'{cwd}/install', exist_ok=True)
    copy_files(f'{pack_path}/_tmp/install', f'{cwd}/install')

    if args.clean:
        if os.path.exists(f'{pack_path}/_tmp'):
            shutil.rmtree(f'{pack_path}/_tmp')
        rm_file(f'{sign_path}/ota_signed.bin')
        rm_file(f'{sign_path}/compress.bin')
        rm_file(f'{sign_path}/temp_after_compress')
        rm_file(f'{sign_path}/temp_before_compress')
        rm_file(f'{sign_path}/primary_all_signed.bin')
        

    logging.debug("\r\nFlashing all-app.bin and otp_efuse_config.json to device:")
    logging.debug(f"\r\n\tbk_loader.exe download --portinfo 18 --baudrate 1500000 --infile all-app.bin --aes-key {args.flash_aes_key}\r\n")

if __name__ == '__main__':
    logging.basicConfig()
    logging.getLogger().setLevel(logging.DEBUG)
    stream_handler = logging.StreamHandler(sys.stdout)
    stream_handler.setLevel(logging.DEBUG)
    main()
