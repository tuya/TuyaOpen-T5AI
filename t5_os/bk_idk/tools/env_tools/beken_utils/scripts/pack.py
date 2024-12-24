#!/usr/bin/env python3

import logging
from .gen_ppc import *
from .gen_mpc import *
from .gen_security import *
from .gen_ota import *
from .gen_otp import *
from .bl1_sign import bl1_sign
from .bl2_sign import bl2_sign
from scripts.partition import *
from .compress import *
from .gen_ppc import *

def pack_all(config_dir, aes_key=None, pk_hash=True):
    if config_dir != None:
        logging.debug(f'cd {config_dir}')
        os.chdir(config_dir)

    gen_ppc_config_file('ppc.csv', 'gpio_dev.csv', '_ppc.h')

    o = OTA('ota.csv')
    s = Security('security.csv')

    if aes_key != None:
        flash_aes_key = aes_key
    else:
        flash_aes_key = s.flash_aes_key 

    ota_type = o.get_strategy()
    boot_ota = o.get_boot_ota()
    app_version = o.get_version()

    p = Partitions('partitions.csv', ota_type, boot_ota, s.secureboot_en)

    p.gen_bins_for_bl2_signing(app_version)

    with open('pack.json', 'r') as f:
        pack_json_data = json.load(f)
        if len(pack_json_data) == 0:
            logging.debug(f'pack.json is empty, not pack all-app.bin/ota.bin')
            return

    gen_otp_efuse_config_file(s.flash_aes_type, flash_aes_key, s.bl2_root_pubkey, s.secureboot_en, o.get_boot_ota(),'otp_efuse_config.json')

    with open('pack.json', 'r') as f:
        pack_json_data = json.load(f)
        if len(pack_json_data) == 0:
            logging.debug(f'pack.json is empty, not pack all-app.bin/ota.bin')
            return

    gen_otp_efuse_config_file(s.flash_aes_type, flash_aes_key, s.bl2_root_pubkey, s.secureboot_en, o.get_boot_ota(),'otp_efuse_config.json')

    pbl2 = p.find_partition_by_name('bl2')
    bl2_ver = o.get_bl2_version()
    if pbl2 != None:
        if s.secureboot_en:
            bl1_sign('sign', s.bl2_root_key_type, s.bl2_root_privkey, s.bl2_root_pubkey, None, pbl2.bin_name, pbl2.static_addr, pbl2.load_addr, 'primary_manifest.bin', bl2_ver)
            cmd = f'beken_utils/main.py sign bl1_sign --action_type sign --key_type {s.bl2_root_key_type} --privkey_pem_file {s.bl2_root_privkey} --pubkey_pem_file {s.bl2_root_pubkey} --bin_file {pbl2.bin_name} --static_addr {pbl2.static_addr} --load_addr {pbl2.load_addr} --outfile primary_manifest.bin'
            save_cmd("Sign bootloader", cmd)
            if (boot_ota == True):
                pbl2_B = p.find_partition_by_name('bl2_B')
                bl1_sign('sign', s.bl2_root_key_type, s.bl2_root_privkey, s.bl2_root_pubkey, None, pbl2_B.bin_name, pbl2_B.static_addr, pbl2_B.load_addr, 'secondary_manifest.bin', bl2_ver)
                cmd = f'beken_utils/main.py bl1_sign --action_type sign --key_type {s.bl2_root_key_type} --privkey_pem_file {s.bl2_root_privkey} --pubkey_pem_file {s.bl2_root_pubkey} --bin_file {pbl2_B.bin_name} --static_addr {pbl2_B.static_addr} --load_addr {pbl2_B.load_addr} --outfile secondary_manifest.bin'
                save_cmd("sign bootloader B", cmd)

            if os.path.exists('bl2_B.bin'):
                pota = p.find_partition_by_name('ota')
                pota.bin_name = 'bl2_B.bin'
                pota.load_addr = "0x{:08x}".format(pota.flash_base_addr + phy2virtual(ceil_align(pota.partition_offset, CRC_UNIT_TOTAL_SZ)))
                pota.static_addr = pota.load_addr
                bl1_sign('sign', s.bl1_root_key_type, s.bl1_root_privkey, s.bl1_root_pubkey, None, pota.bin_name, pota.static_addr, pota.load_addr, 'secondary_manifest.bin', bl2_ver)
                cmd = f'beken_utils/main.py bl1_sign --action_type sign --key_type {s.bl2_root_key_type} --privkey_pem_file {s.bl2_root_privkey} --pubkey_pem_file {s.bl2_root_pubkey} --bin_file bl2_B.bin --static_addr {pota.static_addr} --load_addr {pota.load_addr} --outfile secondary_manifest.bin'
                save_cmd("sign bootloader B", cmd)
        pall = p.find_partition_by_name('primary_all')
        if s.update_key_en:
            bl2_sign('sign', s.bl2_root_key_type, s.bl2_new_privkey, s.bl2_new_pubkey, None, 'primary_all.bin', pall.vir_sign_size, app_version, o.get_app_security_counter(), 'primary_all_signed.bin', 'app_hash.json')
        else:
            bl2_sign('sign', s.bl2_root_key_type, s.bl2_root_privkey, s.bl2_root_pubkey, None, 'primary_all.bin', pall.vir_sign_size, app_version, o.get_app_security_counter(), 'primary_all_signed.bin', 'app_hash.json')

        if (ota_type == 'OVERWRITE'):
            compress_bin('primary_all_signed.bin', 'compress.bin')
            pota = p.find_partition_by_name('ota')
            bl2_sign('sign', s.bl2_root_key_type, s.bl2_root_privkey, s.bl2_root_pubkey, None, 'compress.bin', pota.partition_size, app_version, o.get_app_security_counter(), 'ota_signed.bin', 'ota_hash.json')
        elif (ota_type == 'XIP'):
            pota = p.find_partition_by_name('primary_all')
            bl2_sign('sign', s.bl2_root_key_type, s.bl2_root_privkey, s.bl2_root_pubkey, None, 'primary_all.bin', pall.vir_sign_size, app_version, o.get_app_security_counter(), 'ota_signed.bin', 'ota_hash.json')

    p.pack_bin('pack.json', s.flash_aes_type, s.single_bin, flash_aes_key, o.get_app_security_counter(),o.get_encrypt(), o.get_boot_ota(), pk_hash)
    p.install_bin()
