#!/usr/bin/env python3
# coding=utf-8
# 参数说明：
# $1 - params path: $1/build_param.[cmake/config/json]
# $2 - user cmd: build/clean/...

import os
import sys
import json

from tools.util import (
    copy_file, need_settarget, record_target,
    calc_md5sum, do_subprocess, get_system_name
)
from tools.do_with_assets import do_with_assets


def clean(build_root):
    '''
    make clean
    make clean -C ./bk_idk/
    '''
    cmd = f"cd {build_root} && make clean"
    do_subprocess(cmd)

    bk_idf_path = os.path.join(build_root, "bk_idk")
    cmd = f"cd {bk_idf_path} && make clean"
    do_subprocess(cmd)
    pass


def parser_para_file(json_file):
    if not os.path.isfile(json_file):
        print(f"Error: Not found [{json_file}].")
        return {}
    try:
        f = open(json_file, 'r', encoding='utf-8')
        json_data = json.load(f)
        f.close()
    except Exception as e:
        print(f"Parser json error:  [{str(e)}].")
        return {}
    return json_data


def set_environment(root, build_param_path, param_data):
    os.environ["TUYA_PROJECT_DIR"] = root
    os.environ["BUILD_PARAM_DIR"] = build_param_path
    os.environ["TUYA_HEADER_DIR"] = param_data["OPEN_HEADER_DIR"]
    os.environ["TUYA_LIBS_DIR"] = param_data["OPEN_LIBS_DIR"]
    os.environ["TUYA_LIBS"] = param_data["PLATFORM_NEED_LIBS"]
    os.environ["TUYA_APP_NAME"] = param_data["CONFIG_PROJECT_NAME"]
    open_root = param_data["OPEN_ROOT"]
    board_path = os.path.join(open_root, "boards", "ESP32")
    os.environ["TUYAOS_BOARD_PATH"] = board_path
    pass


def check_bootloader_bin(build_root) -> bool:
    boot_file = os.path.join(build_root, "bk_idk",
                             "components", "bk_libs",
                             "bk7258", "bootloader",
                             "normal_bootloader",
                             "bootloader.bin")
    real_md5sum = calc_md5sum(boot_file)
    exp_md5sum = "f8f45b0779a8269fa089ac84ebd9c149"
    if real_md5sum != exp_md5sum:
        print(f"Error md5sum: real[{real_md5sum}], expectation[{exp_md5sum}]")
        return False

    return True


def setup_build(root, build_root, build_param_path, param_data):
    set_environment(root, build_param_path, param_data)

    gen_file = os.path.join(build_root, "bk_idk",
                            "tools", "build_tools",
                            "part_table_tools",
                            "config", "gen_files_list.txt")
    with open(gen_file, "w", encoding='utf-8') as f:
        f.write("")

    if not check_bootloader_bin(build_root):
        print("Error: check bootloader bin failed.")
        return False

    return True


def build(build_root, target, app_name, app_ver) -> bool:
    cmd = f"cd {build_root} && make {target}"

    project_dir = os.path.join("..", "projects", "tuya_app")
    cmd += f" PROJECT_DIR={project_dir}"

    build_dir = os.path.join("..", "build")
    cmd += f" BUILD_DIR={build_dir}"

    cmd += f" APP_NAME={app_name}"
    cmd += f" APP_VERSION={app_ver}"
    cmd += " -j"

    ret = do_subprocess(cmd)

    if ret != 0:
        print("Error: Build error.")
        return False
    return True


def _merge_bin(pad_size, app_bin, app1_bin, ua_file_bin):
    with open(ua_file_bin, 'wb') as outfile:
        with open(app_bin, 'rb') as f:
            outfile.write(f.read())
        outfile.write(b'\xff' * pad_size)
        with open(app1_bin, 'rb') as f:
            outfile.write(f.read())
    pass


def copy_assets(build_root, target, param_data):
    build_path = os.path.join(build_root, "build", target)
    app_all_bin = os.path.join(build_path, "all-app.bin")
    if not os.path.exists(app_all_bin):
        print(f"Error: Not found {app_all_bin}.")
        return False

    app_bin = os.path.join(build_path, "app.bin")
    bin_size = os.stat(app_bin).st_size
    max_size = 1740800
    if bin_size > max_size:
        print(f"Error: bin file is too big, limit {max_size}, act {bin_size}.")
        return False

    pad_size = max_size - bin_size
    ua_file_bin = os.path.join(build_path, "ua_file.bin")
    app1_bin = os.path.join(build_path, "app1.bin")
    _merge_bin(pad_size, app_bin, app1_bin, ua_file_bin)
    total_size = os.stat(ua_file_bin).st_size

    print(f"ofs: {bin_size}")
    print(f"pad_bytes_size: {pad_size}")
    print(f"total_size: {total_size}")

    project_dir = os.path.join(
        build_root, "projects", "tuya_app")
    format_bin_py = os.path.join(
        project_dir, "tuya_scripts", "format_up_bin.py")
    app_ug_bin = os.path.join(build_path, "app_ug.bin")
    cmd = f"python {format_bin_py} {ua_file_bin} {app_ug_bin} \
500000 1000 0 1000 18D0 {max_size}"
    if 0 != do_subprocess(cmd):
        print("Error: format_bin_py failed.")
        return False

#     diff_ota_bin_py = os.path.join(project_dir, "tuya_scripts", "diff2ya.py")
#     app_ota_ug_bin = os.path.join(build_path, "app_ota_ug.bin")
#     cmd = f"python {diff_ota_bin_py} {app_ug_bin} \
# {app_ug_bin} {app_ota_ug_bin} 0"
#     if 0 != do_subprocess(cmd):
#         print("Error: diff_ota_bin_py failed.")
#         return False

    app_name = param_data["CONFIG_PROJECT_NAME"]
    app_ver = param_data["CONFIG_PROJECT_VERSION"]
    output_path = param_data["BIN_OUTPUT_DIR"]
    os.makedirs(output_path, exist_ok=True)
    try:
        copy_file(app_all_bin,
                  os.path.join(output_path, f"{app_name}_QIO_{app_ver}.bin"))
        copy_file(ua_file_bin,
                  os.path.join(output_path, f"{app_name}_UA_{app_ver}.bin"))
        # copy_file(app_ota_ug_bin,
        #           os.path.join(output_path, f"{app_name}_UG_{app_ver}.bin"))
        copy_file(os.path.join(build_path, "app.elf"),
                  os.path.join(output_path, f"{app_name}_{app_ver}.elf"))
        copy_file(os.path.join(build_path, "app.map"),
                  os.path.join(output_path, f"{app_name}_{app_ver}.map"))
        copy_file(os.path.join(build_path, "app.nm"),
                  os.path.join(output_path, f"{app_name}_{app_ver}.nm"))
        copy_file(os.path.join(build_path, "app.txt"),
                  os.path.join(output_path, f"{app_name}_{app_ver}.txt"))
        copy_file(os.path.join(build_path, "size_map.txt"),
                  os.path.join(output_path, "size_map.txt"))
        copy_file(os.path.join(build_path, "size_map_detail.csv"),
                  os.path.join(output_path, "size_map_detail.csv"))
        copy_file(os.path.join(build_path, "size_map_total.csv"),
                  os.path.join(output_path, "size_map_total.csv"))
    except Exception as e:
        print(f"Error: copy assets: {str(e)}")
        return False

    return True


def do_with_compile(root, build_root, user_cmd,
                    target, build_param_path, param_data):
    # Setup build
    if not setup_build(root, build_root, build_param_path, param_data):
        sys.exit(1)

    # clean project
    app_target_file = os.path.join(build_root, ".app")
    app_name = param_data["CONFIG_PROJECT_NAME"]
    if "clean" == user_cmd or need_settarget(app_target_file, app_name):
        clean(build_root)
        if "clean" == user_cmd:
            sys.exit(0)

    # build project
    record_target(app_target_file, app_name)
    app_ver = param_data["CONFIG_PROJECT_VERSION"]
    if not build(build_root, target, app_name, app_ver):
        sys.exit(1)
    pass


def main():
    '''
    1. 提前配置一些环境变量
    1. 如果编译的项目变化，则清理现场
    1. 拷贝产物到输出路径中
    '''
    if len(sys.argv) < 2:
        print(f"Error: At least 2 parameters are needed {sys.argv}.")
    build_param_path = sys.argv[1]
    user_cmd = sys.argv[2]
    target = "bk7258"

    root = os.path.dirname(os.path.abspath(__file__))
    build_root = os.path.join(root, "t5_os")
    build_param_file = os.path.join(build_param_path, "build_param.json")
    param_data = parser_para_file(build_param_file)
    if not len(param_data):
        sys.exit(1)

    if "windows" == get_system_name():
        do_with_assets(root, build_root, user_cmd,
                       target, param_data)
    else:
        do_with_compile(root, build_root, user_cmd, target,
                        build_param_path, param_data)

    # copy asset
    if not copy_assets(build_root, target, param_data):
        sys.exit(1)

    sys.exit(0)


if __name__ == "__main__":
    main()
