Overview
-----------------------

This example create all-app.bin by running run.sh (Linux) or run.bat (Windows). To be specific, it demos how to:

 - create signed app bin (primary_all_signed.bin) by signing raw app bin (primary_app.bin).
 - create signed OTA bin (ota_sign.bin) by signing raw app bin (primary_app.bin) with different version
 - generate nvs_key.bin if nvs_key.bin not exists; use existing nvs_key.bin otherwise
 - encrypt bl2.bin, primary_all_signed.bin and nvs_key.bin with beken flash AES key, and then add CRC to them
 - create nvs.bin from nvs.csv, then encrypt nvs.bin with nvs_key.bin
 - encrypt wiz_mfr.bin with nvs_key.bin
 - create all-app.bin per pack.json

Assumption
-------------------------
Each sub-directory (build_server, sign_server, pack_server) in this directory can be a sperate real server. Following assumption should be meet before
we can correctly run run.py:

 - beken_utils is correctly deployed in sign server and pack server
 - the private PEM key file exists in sign server
 - the immutable configuration file nvs.csv, partitions.csv, pack.csv and public PEM key file exists in pack server
 - the factory data wiz_mfr.bin exists in pack server
 - if each device use same NVS key, then nvs_key.bin also need to be existed in pack server

How to run example
-------------------------

Example:

    python3 -B run.py --flash_aes_key 73c7bf397f2ad6bf4e7403a7b965dc5ce0645df039c2d69c814ffb403183fb18 --app_version 0.0.1 --app_security_counter 5 --ota_version 0.0.3 --ota_security_counter 5 --ota_type XIP --pubkey root_ec256_pubkey.pem --clean

If the beken_utils is located in idk/tools/env_tools/, you can build bootloader (bl2.bin) and app (primary_all.bin) from idk/projects/security/secureboot_no_tfm, and then generate all-app.bin:

    python3 -B run.py --flash_aes_key 73c7bf397f2ad6bf4e7403a7b965dc5ce0645df039c2d69c814ffb403183fb18 --app_version 0.0.1 --app_security_counter 5 --ota_version 0.0.3 --ota_security_counter 5 --ota_type XIP --pubkey root_ec256_pubkey.pem --clean --build

You can also run run.sh or run.bat directly. After the command run successfully, the result is in install directory:

 - all-app.bin - combines all partition binary into a single binary blob.
 - ota.bin - the binary used for OTA
 - pack_cmd_list.txt - the detailed commands used to sign, encrypt and pack binaries.

How to integrate the tool to your owner tool
--------------------------------------------------------

Take following steps to integrate the beken pack tool into your tool:

 - Check run.py to gain overview understanding about the pack flow
 - Port sign_server/sign.py to your real sign server
 - Port pack_server/pack.py to your real pack server

Where to find the download tool: bk_loader
---------------------------------------------------------

The beken download tool: bk_loader is placed under: idk/tools/env_tools/beken_utils/tools/bk_loader_pi.

Some useful commands:

 - download command: ./bk_loader download -p 0 -i ./all_app.bin --reboot 0
 - read uid: ./bk_loader readinfo -p 0 --read-uid --reboot 0
 - read flash: ./bk_loader readinfo -p 0 -f 1000-20 --reboot 0
 - reboot: ./bk_loader readinfo -p 0 --reboot 1

Where to find the NVS tool
--------------------------------------------------------
The third-party NVS tool is place in: tools/env_tools/beken_utils/scripts/NVS

