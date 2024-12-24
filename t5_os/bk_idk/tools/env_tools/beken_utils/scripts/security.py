#!/usr/bin/env python3
import os
import logging
import csv

from .common import *
from .parse_csv import *
from cryptography.hazmat.primitives import serialization

security_keys = [
    'secureboot_en',
    'flash_aes_type',
    'single_bin',
    'flash_aes_key',
    'root_key_type',
    'root_pubkey',
    'root_privkey',
    'new_pubkey',
    'new_privkey',
    'update_key',
]

class Security(dict):

    def __getitem__(self, key):
        return self.csv.dic[key]

    def __init__(self, csv_file):
        self.csv = Csv(csv_file, False, security_keys)
        self.parse_csv()

    def is_flash_aes_fixed(self):
        return (self.flash_aes_type.upper() == 'FIXED')

    def is_flash_aes_random(self):
        return (self.flash_aes_type.upper() == 'RANDOM')

    def get_public_bytes_from_pubkeyfile(self, pubkey_file):
        if not os.path.exists(pubkey_file):
            return
        with open(pubkey_file, 'rt') as pem_file:
            first_line = pem_file.readline().strip()
            if first_line != "-----BEGIN PUBLIC KEY-----":
                return
            pem_file.seek(0)
            pem_file_data = pem_file.read()

        try:
            public_key = serialization.load_pem_public_key(pem_file_data.encode())

            der_data = public_key.public_bytes(
                encoding=serialization.Encoding.DER,
                format=serialization.PublicFormat.SubjectPublicKeyInfo)
            return der_data
        except Exception as e:
            logging.error(f'failed to load PEM public key {e}')
            exit(1)

    def parse_csv(self):
        self.secureboot_en = parse_bool(self.csv.dic['secureboot_en'])
        self.flash_aes_type = self.csv.dic['flash_aes_type'].upper()

        self.flash_aes_key = self.csv.dic['flash_aes_key']
        if self.flash_aes_type == 'FIXED':
            key_len = len(self.flash_aes_key)
            if (key_len != 0) and (key_len != 64):
                logging.error(f'Invalid AES key: key length should be 0 or 64')
                exit(1)
        else:
            self.flash_aes_key = None
        self.single_bin = parse_bool(self.csv.dic['single_bin'])
        self.bl1_root_key_type = self.csv.dic['root_key_type']
        self.bl1_root_pubkey = self.csv.dic['root_pubkey']
        self.bl1_root_privkey = self.csv.dic['root_privkey']

        self.bl2_root_key_type = self.csv.dic['root_key_type']
        self.bl2_root_pubkey = self.csv.dic['root_pubkey']
        self.bl2_root_pubkey_bytes = self.get_public_bytes_from_pubkeyfile(self.bl2_root_pubkey)
        self.bl2_root_privkey = self.csv.dic['root_privkey']

        if 'update_key' in self.csv.dic:
            self.update_key_en = parse_bool(self.csv.dic['update_key'])
            if self.update_key_en == True:
                self.bl2_new_pubkey = self.csv.dic['new_pubkey']
                self.bl2_new_pubkey_bytes = self.get_public_bytes_from_pubkeyfile(self.bl2_new_pubkey)
                self.bl2_new_privkey = self.csv.dic['new_privkey']

        if self.secureboot_en == True:
            if (self.bl1_root_key_type != '') and (self.bl1_root_key_type != 'ec256') and (self.bl1_root_key_type != 'rsa2048') and (self.bl1_root_key_type != 'rsa3072'):
                logging.error(f'Unknown root key type {self.root_key_type}, only support ec256, rsa2048')
                exit(1)
