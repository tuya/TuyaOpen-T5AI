#!/usr/bin/env python3

import os
import json
import logging
import shutil
import re
import hashlib
from .common import *
from scripts import partition
from scripts.partition import *

def sign_arg(partition_csv, ota_type, bl1_secureboot_en, boot_ota):
    p = Partitions(partition_csv, ota_type, boot_ota, bl1_secureboot_en)
    logging.debug(f'')
    logging.debug(f'Signing arguments:')

    #p.gen_bins_for_bl2_signing()

    pbl2 = p.find_partition_by_name('bl2')
    if pbl2 != None:
        logging.debug(f'    bl2_load_addr: {pbl2.load_addr}')

    pall = p.find_partition_by_name('primary_all')
    if pall != None:
        logging.debug(f'    app_slot_size: {pall.vir_sign_size}')

    pota = p.find_partition_by_name('ota')
    if pota != None:
        logging.debug(f'    ota_slot_size: {pota.partition_size}')

class Sign:
    def __init__(self, build_dir, tools_dir, prvkey_file=None, digest_file=None, out_file='sign.json'):
        self.tools_dir = tools_dir
        self.build_dir = build_dir
        self.prvkey_file = prvkey_file 
        self.digest_file = digest_file
        self.sig_file = out_file
        
        self.signature_dict = {
            'bl1_sig_s': '',
            'bl1_sig_r': '',
            'bl2_sig_app': '',
            'bl2_sig_ota': ''
        }

        if prvkey_file == None:
            self.prvkey_file = f'{self.build_dir}/root_ec256_privkey.pem'
        else:
            self.prvkey_file = f'{self.build_dir}/{prvkey_file}'

        if os.path.exists(prvkey_file) == False:
            logging.error(f'private key file {prvkey_file} not exists')
            exit(1)

        if digest_file == None:
            self.digest_file = f'{self.build_dir}/digest.json'

        if os.path.exists(digest_file) == False:
            logging.error(f'digest file {digest_file} not exists')
            exit(1)

        with open(self.digest_file, 'r') as f:
            digest = json.load(f)

        #TODO validate key
        self.bl1_manifest_digest = digest['bl1_manifest_digest']
        self.app_digest = digest['app_digest']
        self.ota_digest = digest['ota_digest']

    def ecdsa_sign(self):
        self.bl1_sign()
        self.bl2_sign()

    def bl1_sign(self):
        sh_sec_tools = f'{self.tools_dir}/tools/sh_sec_tools/secure_boot_tool'
        digest_file = '_bl1_manifest_digest.txt'
        with open(digest_file, 'w') as f:
            f.write(self.bl1_manifest_digest)

        cmd = f'{sh_sec_tools} sign -k {self.prvkey_file} -d {digest_file}'
        run_cmd(cmd)
        
        with open('bl1_signature.txt', 'r') as f:
            self.signature_dict['bl1_sig_s'] = f.readline().strip();
            self.signature_dict['bl1_sig_r'] = f.readline();

        with open(self.sig_file, 'w') as f:
            json.dump(self.signature_dict, f, indent=4)
        pass

    def bl2_sign(self):
        bl2_signing_tool_dir = f'{self.tools_dir}/tools/mcuboot_tools/imgtool.py'
        cmd = f'{bl2_signing_tool_dir} ecdsa-sign -k {self.prvkey_file} -d {self.digest_file} -o {self.sig_file}'
        run_cmd(cmd)
        pass
