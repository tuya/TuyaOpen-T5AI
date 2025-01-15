Overview
-----------------------

This example create all-app.bin by running run.sh (Linux) or run.bat (Windows). To be specific, it demos how to:

 - calculate the hash of bootloader binary and app binary
 - create signature from the hash of bootloader and create the signature from the hash of app binary
 - create signed app bin (primary_all_signed.bin) from raw app bin and the signature
 - create signed bootloader bin (bootloader.bin or provision_pack.bin) from raw bl2.bin and the signature
 - compress the OTA binary and calculate the hash of it
 - create signature from the hash of compress OTA, then create the signed ota.bin from compressed OTA binary and signature
 - create all-app.bin per pack.json

Assumption
-------------------------
Each sub-directory (build_server, sign_server, pack_server) in this directory can be a sperate real server. Following assumption should be meet before
we can correctly run run.py:

 - beken_utils is correctly deployed in sign server and pack server
 - the private PEM key file exists in sign server
 - the immutable configuration file security.csv, public/private PEM key file exists in pack server
 - the immutable configuration file ota.csv, security.csv, pack.csv, partitions.csv, public PEM key file and bl2.bin exists in pack server

How to run example
-------------------------

Example:

    python3 -B run.py --app_version 0.0.1 --app_security_counter 5 --ota_version 0.0.3 --ota_security_counter 5 --project security/secureboot_overwrite --clean

If the beken_utils is located in idk/tools/env_tools/, you can build bootloader (bl2.bin) and app (primary_all.bin) from idk/projects/security/secureboot_no_tfm, and then generate all-app.bin:

    python3 -B run.py --app_version 0.0.1 --app_security_counter 5 --ota_version 0.0.3 --ota_security_counter 5 --project security/secureboot_overwrite --clean

After the command run successfully, the result is in install directory:

 - all-app.bin - combines all partition binary into a single binary blob.
 - ota.bin - the binary used for OTA
 - pack_cmd_list.txt - the detailed commands used to pack the binaries.
 - sign_cmd_list.txt - the detailed commands used to sign the binaries.

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
