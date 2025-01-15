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

def copy_file_list(src, dst, flist):
    for f in flist:
        copy_file(f'{src}/{f}', dst)

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

def main():
    parse = argparse.ArgumentParser(description="Sign, encrypt and pack all-app.bin")
    parse.add_argument('--app_version', type=str, required=True, help='Specify app.bin version')
    parse.add_argument('--app_security_counter', type=int, required=True, help='Specify app.bin security counter')
    parse.add_argument('--ota_version', type=str, required=True, help='Specify ota.bin version')
    parse.add_argument('--ota_security_counter', type=str, required=True, help='Specify ota.bin security counter')
    parse.add_argument('--project', type=str, help='Specify the project name')
    parse.add_argument('--build', action='store_true', help='Indicate whether rebuild the project')
    parse.add_argument('--clean', action='store_true', help='Clean all temp files')

    args = parse.parse_args()
    cwd = os.getcwd()
    tool_path = go_back_dir(2)
    build_path = os.path.join(cwd, 'build_server')
    sign_path = os.path.join(cwd, 'sign_server')
    pack_path = os.path.join(cwd, 'pack_server')

    if args.build:
        if args.project == None:
            logging.error(f'option "--project" is required when "--build" is specified')
            exit(1)

    if os.path.exists(f'{pack_path}/_tmp'):
        shutil.rmtree(f'{pack_path}/_tmp')
    os.makedirs(f'{pack_path}/_tmp', exist_ok=True)

    if os.path.exists(f'{sign_path}/_tmp'):
        shutil.rmtree(f'{sign_path}/_tmp')
    os.makedirs(f'{sign_path}/_tmp', exist_ok=True)
   
    if args.build:
        idk_path = go_back_dir(5)
        os.chdir(idk_path)
        build_server_process(args.project)
        project_base = os.path.basename(args.project)

        logging.debug(f'Copy the immutable bin or configurations to pack server')
        files = ['provision.bin', 'primary_manifest.json', 'pack.json', 'partitions.csv', 'ota.csv', 'security.csv', 'root_ec256_pubkey.pem']
        copy_file_list(f'{idk_path}/build/{project_base}/bk7236/_build', f'{pack_path}/immutable', files)
        files = ['security.csv', 'root_ec256_pubkey.pem', 'root_ec256_privkey.pem']
        copy_file_list(f'{idk_path}/build/{project_base}/bk7236/_build', f'{sign_path}/immutable', files)

        copy_file(f'{idk_path}/build/{project_base}/bk7236/_build/otp_efuse_config.json', f'{pack_path}/_tmp')
        copy_file(f'{idk_path}/build/{project_base}/bk7236/_build/ota_vmin_signed.bin', f'{pack_path}/_tmp')
        copy_file(f'{idk_path}/build/{project_base}/bk7236/_build/jump.bin', f'{pack_path}/_tmp')
        copy_file(f'{idk_path}/build/{project_base}/bk7236/_build/bl2.bin', f'{pack_path}/_tmp')
        copy_file(f'{idk_path}/build/{project_base}/bk7236/_build/bl2_B.bin', f'{pack_path}/_tmp')
        copy_file(f'{idk_path}/build/{project_base}/bk7236/_build/tfm_s.bin', f'{pack_path}/_tmp')
        copy_file(f'{idk_path}/build/{project_base}/bk7236/_build/cpu0_app.bin', f'{pack_path}/_tmp')

    copy_files(f'{pack_path}/immutable', f'{pack_path}/_tmp')
    copy_files(f'{sign_path}/immutable', f'{sign_path}/_tmp')

    os.chdir(f'{pack_path}/_tmp')
    bin_hash_list = pack_server_get_app_bin_hash(tool_path)

    os.chdir(f'{sign_path}/_tmp')
    bin_sig_list = sign_server_sign_app_bin_hash(tool_path, bin_hash_list[0], bin_hash_list[1], bin_hash_list[2])

    os.chdir(f'{pack_path}/_tmp')
    pack_server_sign_from_app_sig(tool_path, bin_sig_list[0], bin_sig_list[1], bin_sig_list[2], bin_sig_list[3], bin_sig_list[4])
    ota_bin_hash = pack_server_get_ota_bin_hash(tool_path)

    os.chdir(f'{sign_path}/_tmp')
    ota_bin_sig = sign_server_sign_ota_bin_hash(tool_path, ota_bin_hash)

    os.chdir(f'{pack_path}/_tmp')
    pack_server_sign_from_ota_sig(tool_path, ota_bin_sig)
    pack_server_pack(tool_path)

    os.chdir(cwd)
    if os.path.exists(f'{cwd}/install'):
        shutil.rmtree(f'{cwd}/install')
    os.makedirs(f'{cwd}/install', exist_ok=True)
    copy_files(f'{pack_path}/_tmp/install', f'{cwd}/install')
    copy_file(f'{sign_path}/_tmp/sign_cmd_list.txt', f'{cwd}/install')

    if args.clean:
        if os.path.exists(f'{pack_path}/_tmp'):
            shutil.rmtree(f'{pack_path}/_tmp')

        if os.path.exists(f'{sign_path}/_tmp'):
            shutil.rmtree(f'{sign_path}/_tmp')

    print("\r\nFlashing all-app.bin and otp_efuse_config.json to device:")
    print(f"\r\n\tbk_loader.exe download --portinfo 18 --baudrate 1500000 --infile all-app.bin\r\n")

if __name__ == '__main__':
    logging.basicConfig()
    logging.getLogger().setLevel(logging.DEBUG)
    stream_handler = logging.StreamHandler(sys.stdout)
    stream_handler.setLevel(logging.DEBUG)
    main()
