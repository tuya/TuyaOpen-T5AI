.. _bk_config_overview:

System Configuration
==========================

:link_to_translation:`zh_CN:[中文]`

The Armino configuration is integrated in the build, and the process is shown in the following figure:

.. figure:: picture/config_overview.svg
     :align: center
     :alt: 8
     :figclass: align-center

The processing corresponding to the labels in the figure is:

  - 1. Code generation, before compiling, the build system scans the configuration file first, and generates the header file or configuration file required for compiling.
  - 2. Generate BIN - compiles, links and generates raw binaries.
  - 3. Scan the configuration file again, pack according to the configuration items, including adding CRC, encrypting, signing, etc., and finally generate all.bin and ota.bin.
  - 4. Deploy the generated OTP/EFUSE configuration file to the board.

Configuration Files
------------------------

Armino supports the following configuration files:

  - Kconfig - component configuration, generates sdkconfig.h. For details, refer to :ref:`Kconfig configuration <bk_config_kconfig>`.
  - csv - configuration file in CSV format, including:

    - partitions.csv - partition configuration table, used to generate partitions_gen.h. Please refer to :ref:`Partition configuration <bk_config_partitions>` for details.
    - mpc.csv - Security block peripheral configuration table, used to generate security.h. For details, please refer to :ref:`Security Configuration <bk_config_security>`.
    - ppc.csv - Security peripheral configuration table, used to generate security.h. For details, please refer to :ref:`Security Configuration <bk_config_security>`.
    - security.csv - security configuration, used to generate security.h and otp_efuse_otp.json, also used for packaging. For details, please refer to :ref:`Security Configuration <bk_config_security>`. .
  - key - Configure security-related KEY, such as FLASH AES KEY, secure boot public key pair, etc. For details, please refer to :ref:`Security Configuration <bk_config_security>`.

Configuration Files Organization
-------------------------------------

The configuration file is organized in Armino as shown in the following figure:

::

     -armino/
         - components/
             - c1/
                 -Kconfig
         - middleware/
             -boards/
                 - bk7239/
                     -csv
                         - mpc.csv
                         -ppc.csv
                         -ota.csv
                         - partitions.csv
                         - security.csv
                     - key
                         - root_ec256_pubkey.pem
                         - root_ec256_privkey.pem
             - soc/
                 - bk7239/
                     -bk7239.defconfig
         - projects/
             - my_project/
                 -config/
                     -bk7239/config
                     - csv
                         - mpc.csv
                         -ppc.csv
                         -ota.csv
                         - partitions.csv
                         - security.csv
                         - bk7239_partitions.csv
                     - key
                         - root_ec256_pubkey.pem
                         - root_ec256_privkey.pem
                 -Kconfig.projbuild
                 - main/
                     -Kconfig
                 - components/
                     - c1/
                         -Kconfig

For configuration files or configuration items with the same name, the priority rules are:

  - Project Configuration > Middleware Configuration > Component Configuration.
  - For configuration files in the same directory: specific chip configuration items > general configuration items.

Take the directory shown in the above figure as an example, projects/my_project/csv/bk7239_partitions.csv will be used as the partition table, and projects/my_project/csv/security.csv will be used as the
Security configuration table.

.. note::

   Kconfig uses the configuration items in the high priority file to replace the configuration items in the low priority configuration file;
   Other configuration files are replaced by configuration files with higher priority than those with lower priority.
