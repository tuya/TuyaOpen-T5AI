#!/usr/bin/env python3

import logging
from .gen_ppc import *
from .gen_mpc import *
from .gen_security import *
from .gen_ota import *
from .gen_otp import *
from .bl1_sign import *
from .bl2_sign import *
from .partition import *
from .compress import *
from .gen_ppc import *
from .pk_hash import *

def check_exist(file_name):
    if not os.path.exists(file_name):
        logging.error(f'{file_name} NOT exists')
        exit(1)

def check_config():
    check_exist('bl2.bin')
    check_exist('tfm_s.bin')
    check_exist('cpu0_app.bin')
    check_exist('security.csv')
    check_exist('ota.csv')
    check_exist('partitions.csv')

def get_hash(file_name):
    with open(file_name, 'r') as f:
        d = json.load(f)
        return d['hash']

def get_app_sig(file_name):
    with open(file_name, 'r') as f:
        d = json.load(f)
        return d['signature']

#Step1 - get hash of app binary and hash of manifest
def get_app_bin_hash():
    check_config()

    #gen_ppc_config_file('ppc.csv', 'gpio_dev.csv', '_ppc.h')
    o = OTA('ota.csv')
    s = Security('security.csv')
    ota_type = o.get_strategy()
    app_version = o.get_version()
    p = Partitions('partitions.csv', ota_type, False, s.secureboot_en)
    p.gen_bins_for_bl2_signing(app_version)

    pbl2 = p.find_partition_by_name('bl2')
    bl2_ver = o.get_bl2_version()
    bl1_sign('hash', s.bl1_root_key_type, s.bl1_root_privkey, s.bl1_root_pubkey, None, pbl2.bin_name, pbl2.load_addr, pbl2.static_addr, 'primary_manifest.bin', bl2_ver)
    if os.path.exists('bl2_B.bin'):
        pota = p.find_partition_by_name('ota')
        pota.bin_name = 'bl2_B.bin'
        pota.load_addr = "0x{:08x}".format(pota.flash_base_addr + phy2virtual(ceil_align(pota.partition_offset, CRC_UNIT_TOTAL_SZ)))
        pota.static_addr = pota.load_addr
        bl1_sign('hash', s.bl1_root_key_type, s.bl1_root_privkey, s.bl1_root_pubkey, None, pota.bin_name, pota.load_addr, pota.static_addr, 'secondary_manifest.bin', bl2_ver)
    pall = p.find_partition_by_name('primary_all')
    if s.update_key_en:
        bl2_sign('hash', s.bl2_root_key_type, s.bl2_new_privkey, s.bl2_new_pubkey_bytes, None, 'primary_all.bin', pall.vir_sign_size, app_version, o.get_app_security_counter(), 'primary_all_signed.bin', 'app_hash.json')
    else:
        bl2_sign('hash', s.bl2_root_key_type, s.bl2_root_privkey, s.bl2_root_pubkey_bytes, None, 'primary_all.bin', pall.vir_sign_size, app_version, o.get_app_security_counter(), 'primary_all_signed.bin', 'app_hash.json')

#Step2 - generate signature from app/manifest hash, do it in server has private key
def sign_app_bin_hash(bl2_bin_hash=None, bl2_b_bin_hash=None, app_bin_hash=None):
    s = Security('security.csv')
    if bl2_bin_hash == None:
        bl2_bin_hash = get_hash('primary_manifest_hash.json')
    if bl2_b_bin_hash == None:
        bl2_b_bin_hash = get_hash('secondary_manifest_hash.json')
    if app_bin_hash == None:
        app_bin_hash = get_hash('app_hash.json')

    bl1_sign_hash(s.bl1_root_privkey, bl2_bin_hash, 'primary_manifest_sig.json')
    bl1_sign_hash(s.bl1_root_privkey, bl2_b_bin_hash, 'secondary_manifest_sig.json')
    if s.update_key_en:
        bl2_sign_hash(s.bl2_new_privkey, app_bin_hash, 'app_sig.json')
    else:
        bl2_sign_hash(s.bl2_root_privkey, app_bin_hash, 'app_sig.json')

#Step3 - generate signed bin from signature
def sign_from_app_sig(bl2_sig_r, bl2_sig_s, bl2_b_sig_r, bl2_b_sig_s, app_sig):
    o = OTA('ota.csv')
    s = Security('security.csv')
    ota_type = o.get_strategy()
    app_version = o.get_version()
    p = Partitions('partitions.csv', ota_type, False, s.secureboot_en)

    pbl2 = p.find_partition_by_name('bl2')
    bl2_ver = o.get_bl2_version()
    if s.secureboot_en:
        with open('bl1_signature.txt', 'w') as f:
            f.write(bl2_sig_r)
            f.write("\n")
            f.write(bl2_sig_s)

        with open('bl1_b_signature.txt', 'w') as f:
            f.write(bl2_b_sig_r)
            f.write("\n")
            f.write(bl2_b_sig_s)

        bl1_sign('sign_from_sig', s.bl1_root_key_type, s.bl1_root_privkey, s.bl1_root_pubkey, None, pbl2.bin_name, pbl2.load_addr, pbl2.static_addr, 'primary_manifest.bin', bl2_ver)

        if os.path.exists('bl2_B.bin'):
            pota = p.find_partition_by_name('ota')
            pota.bin_name = 'bl2_B.bin'
            pota.load_addr = "0x{:08x}".format(pota.flash_base_addr + phy2virtual(ceil_align(pota.partition_offset, CRC_UNIT_TOTAL_SZ)))
            pota.static_addr = pota.load_addr
            bl1_sign('sign_from_sig', s.bl1_root_key_type, s.bl1_root_privkey, s.bl1_root_pubkey, None, pota.bin_name, pota.load_addr, pota.static_addr, 'secondary_manifest.bin', bl2_ver)       
    pall = p.find_partition_by_name('primary_all')
    if s.update_key_en:
        bl2_sign('sign_from_sig', s.bl2_root_key_type, s.bl2_new_privkey, s.bl2_new_pubkey_bytes, app_sig, 'primary_all.bin', pall.vir_sign_size, app_version, o.get_app_security_counter(), 'primary_all_signed.bin', 'app_hash.json')
    else:
        bl2_sign('sign_from_sig', s.bl2_root_key_type, s.bl2_root_privkey, s.bl2_root_pubkey_bytes, app_sig, 'primary_all.bin', pall.vir_sign_size, app_version, o.get_app_security_counter(), 'primary_all_signed.bin', 'app_hash.json')

#Step4 - get hash of ota binary
def get_ota_bin_hash():
    o = OTA('ota.csv')
    s = Security('security.csv')
    ota_type = o.get_strategy()
    app_version = o.get_version()
    p = Partitions('partitions.csv', ota_type, False, s.secureboot_en)

    if (ota_type == 'OVERWRITE'):
        compress_bin('primary_all_signed.bin', 'compress.bin')
        pota = p.find_partition_by_name('ota')
        bl2_sign('hash', s.bl2_root_key_type, s.bl2_root_privkey, s.bl2_root_pubkey, None, 'compress.bin', pota.partition_size, app_version, o.get_app_security_counter(), 'ota_signed.bin', 'ota_hash.json')

#Step5 - generate signature from ota bin hash, do it in server has private key
def sign_ota_bin_hash(ota_hash):
    s = Security('security.csv')
    bl2_sign_hash(s.bl2_root_privkey, ota_hash, 'ota_sig.json')

#Step6 - generate ota.bin from ota signature
def sign_from_ota_sig(ota_bin_sig):
    o = OTA('ota.csv')
    s = Security('security.csv')
    ota_type = o.get_strategy()
    app_version = o.get_version()
    p = Partitions('partitions.csv', ota_type, False, s.secureboot_en)

    if (ota_type == 'OVERWRITE'):
        pota = p.find_partition_by_name('ota')
        bl2_sign('sign_from_sig', s.bl2_root_key_type, s.bl2_root_privkey, s.bl2_root_pubkey, ota_bin_sig, 'compress.bin', pota.partition_size, app_version, o.get_app_security_counter(), 'ota_signed.bin', 'ota_hash.json')

#Step7 - pack download bin
def steps_pack(aes_key_type=None, aes_key=None, bl1_secureboot_en=False, ota_type='XIP', ota_security_counter=0, ota_encrypt='FALSE', boot_ota=False, bl2_pubkey_pem_file='root_ec256_pubkey.pem', bl2_version="v1.0.0"):
    if aes_key_type != None:
        flash_aes_type = aes_key_type
        flash_aes_key = aes_key
    elif os.path.exists('security.csv'):
        s = Security('security.csv')
        flash_aes_key = s.flash_aes_key 
        flash_aes_type = s.flash_aes_type
    else:
        flash_aes_type = 'NONE'
        flash_aes_key = ''

    logging.debug(f'steps_pack, bl1_secureboot_en={bl1_secureboot_en}, ota_type={ota_type}, ota_security_counter={ota_security_counter}, ota_encrypt={ota_encrypt}, bl2_pubkey_pem_file={bl2_pubkey_pem_file}')
    p = Partitions('partitions.csv', ota_type, boot_ota, bl1_secureboot_en, bl2_version)
    p.pack_bin('pack.json', flash_aes_type, s.single_bin, flash_aes_key, ota_security_counter, ota_encrypt, boot_ota)
    p.install_bin()

def steps_pack_csv():
    o = OTA('ota.csv')
    s = Security('security.csv')
    ota_type = o.get_strategy()
    logging.debug(f'steps_pack_csv')

    gen_otp_efuse_config_file(s.flash_aes_type, s.flash_aes_key, s.bl2_root_pubkey, s.secureboot_en, o.get_boot_ota(),'otp_efuse_config.json')

    p = Partitions('partitions.csv', ota_type, False, s.secureboot_en)
    p.pack_bin('pack.json', s.flash_aes_type, s.single_bin, s.flash_aes_key, o.get_app_security_counter(), o.get_encrypt(), False)
    insert_pk_hash('bootloader.bin', s.bl2_root_pubkey)
    p.install_bin()
