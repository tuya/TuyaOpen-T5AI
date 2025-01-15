This directory is used to simulate the sign server.

Import notes!!!
--------------------------------

If the offset or size of primary_cpu0_app in partitions.csv is changed, please update the "--slot-size" of sign command accordingly:

 - build the project
 - update the sign command in sign.py with sign command in idk/build/secureboot_no_tfm/bk7236/_build/install/pack_cmd_list.txt (not idk/tools/env_tools/beken_utils/examples/flash_all_app/install/pack_cmd_list.txt), 
