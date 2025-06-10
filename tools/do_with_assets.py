#!/usr/bin/env python3
# coding=utf-8

import os
import sys
import json
import shutil

from tools.util import (
    get_system_name, do_subprocess, rm_rf
)


def clean(build_root, target):
    rm_rf(build_root)
    print("Cleaning successful for T5AI.")
    return True


def _get_tuya_libs_flag(param_data):
    libs_dir = param_data["OPEN_LIBS_DIR"]
    flag_str = f"-L{libs_dir}"

    libs = param_data["PLATFORM_NEED_LIBS"]
    libs_list = libs.split()
    for lib in libs_list:
        flag_str += f" -l{lib}"

    return flag_str


def _compile_project_elf_src_c(gcc, assets_root, build_root):
    project_elf_src_c = os.path.join(assets_root, "project_elf_src.c")
    obj_file = os.path.join(build_root, "project_elf_src.c.obj")
    ld_flags_file = os.path.join(assets_root, "flags", "ld_flags.txt")

    cmd = f"{gcc} @{ld_flags_file}"
    cmd += f" -c {project_elf_src_c}"
    cmd += f" -o {obj_file}"

    ret = do_subprocess(cmd)

    if ret != 0:
        print("Error: _compile_project_elf_src_c.")
        return False
    return True


def _gen_elf(gcc, assets_root, build_root, target, tuya_libs_flag=""):
    assets_root = os.path.join(assets_root, f"{target}")
    build_root = os.path.join(build_root, f"{target}")

    if not _compile_project_elf_src_c(gcc, assets_root, build_root):
        return False
    ld_flags_file = os.path.join(assets_root, "flags", "ld_flags.txt")
    libs_dir = os.path.join(assets_root, "libs")
    libs_flags_file = os.path.join(assets_root, "flags", "libs_flags.txt")
    elf_file = os.path.join(build_root, "app.elf")
    elf_src_c_o = os.path.join(build_root, "project_elf_src.c.obj")
    ld_file = os.path.join(assets_root, "flags", f"{target}_out.ld")

    cmd = f"{gcc} @{ld_flags_file}"
    cmd += f" {elf_src_c_o}"
    cmd += " -fno-rtti -fno-lto"
    cmd += " -Wl,--start-group"
    cmd += f" -L{libs_dir} @{libs_flags_file}"
    if len(tuya_libs_flag):
        cmd += f" {tuya_libs_flag}"
    cmd += " -Wl,--end-group"
    cmd += f" -T {ld_file}"
    cmd += f" -o {elf_file}"

    ret = do_subprocess(cmd)
    if ret != 0:
        print("Error: _gen_elf.")
        return False

    if not os.path.exists(elf_file):
        print(f"Error: Not found {elf_file}.")
        return False

    return True


def gen_elf_file(gcc, target, assets_root, build_root, param_data):
    tuya_libs_flag = _get_tuya_libs_flag(param_data)

    # cp0
    if not _gen_elf(gcc, assets_root, build_root, f"{target}"):
        return False

    # cp1
    if not _gen_elf(gcc, assets_root, build_root, f"{target}_cp1",
                    tuya_libs_flag):
        return False
    return True


def _elf_2_bin(objcopy, target, build_root):
    build_root = os.path.join(build_root, target)
    elf_file = os.path.join(build_root, "app.elf")
    bin_file = os.path.join(build_root, "app.bin")

    cmd = f"{objcopy} -O binary {elf_file} {bin_file}"

    ret = do_subprocess(cmd)
    if ret != 0:
        print("Error: _elf_2_bin.")
        return False

    if not os.path.exists(bin_file):
        print(f"Error: Not found {bin_file}.")
        return False

    return True


def elf2bin(objcopy, target, build_root):
    # cp0
    if not _elf_2_bin(objcopy, target, build_root):
        return False

    # cp1
    if not _elf_2_bin(objcopy, f"{target}_cp1", build_root):
        return False
    return True


def copy_bin_file(build_root, target):
    from_app1_bin = os.path.join(build_root, f"{target}_cp1", "app.bin")
    to__app1_bin = os.path.join(build_root, f"{target}", "app1.bin")
    shutil.copy(from_app1_bin, to__app1_bin)
    return True


def gen_bootloader_bin(root, build_root, target, assets_root):
    tool_gen_image, _ = _get_packager_tools(assets_root)
    config_file = os.path.join(
        root, "t5_os", "bk_idk", "tools", "env_tools",
        "beken_packager", "config.json"
    )
    bootload_bin = os.path.join(
        root, "t5_os", "bk_idk", "components", "bk_libs",
        target, "bootloader", "normal_bootloader",
        "bootloader.bin")
    out_bootload_bin = os.path.join(
        build_root, target, "bootloader.bin"
    )
    json_file = os.path.join(
        root, "t5_os", "bk_idk", "tools", "env_tools",
        "beken_packager", "partition_bootloader.json"
    )

    cmd = f"{tool_gen_image} genfile -injsonfile {config_file}"
    cmd += f" -infile {bootload_bin}"
    cmd += f" -outfile {out_bootload_bin}"
    cmd += f" -genjson {json_file}"

    ret = do_subprocess(cmd)
    if ret != 0:
        print("Error: gen bootload.bin")
        return False
    return True


def size_format(size, include_size):
    if include_size:
        for (val, suffix) in [(0x400, "K"), (0x100000, "M")]:
            if size % val == 0:
                return "%d%s" % (size // val, suffix)
    return "0x%x" % size


def shrink_val(val, need_shrink):
    return int(val*32/34) if need_shrink else val


def parse_int(v):
    for letter, multiplier in [("k", 1024), ("m", 1024 * 1024)]:
        if v.lower().endswith(letter):
            return parse_int(v[:-1]) * multiplier
        return int(v, 0)


def gen_temp_configuration(asset_config, temp_config):
    with open(asset_config, 'r') as local_json:
        config_data = json.load(local_json)

    start_addr = 0  # default
    for sec in config_data['section']:
        start_addr_tmp = shrink_val(
                parse_int(sec["start_addr"]), True)
        sec['start_addr'] = size_format(start_addr_tmp - start_addr, False)
        sec['size'] = size_format(
            shrink_val(
                parse_int(sec["size"]), True), True)

    temp_data_str = json.dumps(config_data, sort_keys=False, indent=4)
    with open(temp_config, 'w') as f:
        f.write(temp_data_str)
    pass


def _get_packager_tools(assets_root):
    packager_root = os.path.join(assets_root, "packager-tools")
    # "linux", "darwin_x86", "darwin_arm64", "windows"
    sys_name = get_system_name()

    if "linux" == sys_name:
        tool_gen_image = os.path.join(
            packager_root, "linux", "cmake_Gen_image")
        tool_encrypt_crc = os.path.join(
            packager_root, "linux", "cmake_encrypt_crc")
    elif "darwin_x86" == sys_name:
        tool_gen_image = os.path.join(
            packager_root, "mac", "x86_64", "cmake_Gen_image")
        tool_encrypt_crc = os.path.join(
            packager_root, "mac", "x86_64", "cmake_encrypt_crc")
    elif "darwin_arm64" == sys_name:
        tool_gen_image = os.path.join(
            packager_root, "mac", "arm64", "cmake_Gen_image")
        tool_encrypt_crc = os.path.join(
            packager_root, "mac", "arm64", "cmake_encrypt_crc")
    else:
        tool_gen_image = os.path.join(
            packager_root, "windows", "cmake_Gen_image.exe")
        tool_encrypt_crc = os.path.join(
            packager_root, "windows", "cmake_encrypt_crc.exe")

    return tool_gen_image, tool_encrypt_crc


# add 0xff padding to binary file
def add_padding_to_binary(binary_file):
    with open(binary_file, 'ab') as f:
        f.write(bytes([0xff] * 34))  # 34 bytes align


def check_and_padding_binary(app1_bin, app_pack_secs, app_crc_bin):
    last_firmware_size = os.path.getsize(app1_bin)
    last_partition_raw_size = parse_int("320K")
    # 32 bytes align
    last_firmware_aligned_size = (((last_firmware_size + 32 - 1) // 32) * 32)
    # if the last firmware not fill up the partition, add padding value.
    if last_firmware_aligned_size < last_partition_raw_size:
        add_padding_to_binary(app_crc_bin)


def gen_app_all_bin(root, build_root, target, assets_root):
    build_root = os.path.join(build_root, target)
    partitions_config = os.path.join(
        assets_root, target, "flags", "configuration.json")
    temp_config = os.path.join(
        build_root, "configuration_temp.json")
    gen_temp_configuration(partitions_config, temp_config)

    bootload_bin = os.path.join(
        build_root, "bootloader.bin")
    app_bin = os.path.join(build_root, "app.bin")
    app1_bin = os.path.join(build_root, "app1.bin")
    app_all_bin = os.path.join(build_root, "all-app.bin")

    tool_gen_image, tool_encrypt_crc = _get_packager_tools(assets_root)

    cmd = f"{tool_gen_image} genfile -injsonfile {temp_config}"
    cmd += f" -infile {bootload_bin} {app_bin} {app1_bin}"
    cmd += f" -outfile {app_all_bin}"

    ret = do_subprocess(cmd)
    if ret != 0:
        print("Error: gen all-app.bin")
        return False

    cmd = f"{tool_encrypt_crc} -crc {app_all_bin}"

    ret = do_subprocess(cmd)
    if ret != 0:
        print("Error: crc all-app.bin")
        return False

    app_all_crc_bin = os.path.join(build_root, "all-app_crc.bin")
    print(f"[Copy] {app_all_crc_bin} to {app_all_bin}.")
    shutil.copy(app_all_crc_bin, app_all_bin)

    return True


def do_with_assets(root, build_root, user_cmd,
                   target, param_data):
    build_root = os.path.join(build_root, "build")
    if "clean" == user_cmd:
        clean(build_root, target)
        sys.exit(0)

    os.makedirs(os.path.join(build_root, target), exist_ok=True)
    os.makedirs(os.path.join(build_root, f"{target}_cp1"), exist_ok=True)
    assets_root = os.path.join(root, "tools", "vendor-t5_for_open")
    open_root = param_data["OPEN_ROOT"]
    toolchain_root = os.path.join(open_root, "platform", "tools",
                                  "gcc-arm-none-eabi-10.3-2021.10",
                                  "bin")
    toolchain_prefix = "arm-none-eabi-"

    if "windows" == get_system_name():
        gcc = os.path.join(toolchain_root,
                           f"{toolchain_prefix}gcc.exe")
        objcopy = os.path.join(toolchain_root,
                               f"{toolchain_prefix}objcopy.exe")
    else:
        gcc = os.path.join(toolchain_root,
                           f"{toolchain_prefix}gcc")
        objcopy = os.path.join(toolchain_root,
                               f"{toolchain_prefix}objcopy")

    # gen elf: cp0 and cp1
    if not gen_elf_file(gcc, target, assets_root, build_root, param_data):
        sys.exit(1)

    # elf to bin: cp0 and cp1
    if not elf2bin(objcopy, target, build_root):
        sys.exit(1)

    # bootload.bin
    if not gen_bootloader_bin(root, build_root, target, assets_root):
        sys.exit(1)

    copy_bin_file(build_root, target)

    # gen app-all.bin
    if not gen_app_all_bin(root, build_root, target, assets_root):
        sys.exit(1)

    pass
