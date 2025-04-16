#! /usr/bin/env python3
# vim:fenc=utf-8
#
# Copyright © 2025 cc <cc@tuya>
#
# Distributed under terms of the MIT license.

import json
import os
import re
import csv
import sys
from typing import Dict, List, Tuple
import inspect

# ANSI转义码定义
RED = '\033[91m'
GREEN = '\033[92m'
CYAN = '\033[96m'
YELLOW = '\033[93m'
RESET = '\033[0m'

def print_error(message: str):
    sys.stderr.write(f"{RED}Error: {message}{RESET}\n")
    sys.stderr.flush()

def print_warning(message: str):
    print(f"{YELLOW}Warning: {message}{RESET}")

def print_success(message: str):
    print(f"{GREEN}{message}{RESET}")

def print_info(message: str):
    print(f"{CYAN}{message}{RESET}")

def kbytes_str(size: int) -> str:
    return f"{size//1024}K"

def parse_size(value: str) -> int:
    try:
        value = str(value).strip().lower()
        if value in ('y', 'n', 'true', 'false'):
            raise ValueError("布尔值无法转换为尺寸")

        if value.startswith('0x'):
            return int(value, 16)
        elif value.endswith('k'):
            return int(value[:-1]) * 1024
        else:
            return int(value)
    except ValueError as e:
        raise ValueError(f"invalid '{value}': {str(e)}")

def size_to_hex_str(size: int) -> str:
    return f"0x{size:x}"

def load_flash_defaults(csv_path: str) -> Dict[str, int]:
    defaults = {}
    try:
        with open(csv_path, 'r') as f:
            # 预处理CSV内容，过滤注释行
            lines = []
            header_found = False
            for line in f:
                stripped = line.strip()
                # 跳过空行和纯注释行
                if not stripped or stripped.startswith('#'):
                    # 识别列名行（包含'Name,Offset,Size'）
                    if 'Name,Offset,Size' in line:
                        lines.append(line.lstrip('#').strip())  # 去#号并保留列名
                        header_found = True
                    continue
                if not header_found and 'Name' in line and 'Offset' in line and 'Size' in line:
                    lines.append(line.lstrip('#'))
                    header_found = True
                else:
                    lines.append(line)

            #print(f"Processed CSV lines:\n{chr(10).join(lines)}")  # 打印处理后的内容

            reader = csv.DictReader(lines)
            for row in reader:
                try:
                    name = row['Name'].strip().lower()
                    size_str = row['Size'].strip()
                    print(f"Processing row - Name: {name}, Size: {size_str}")  # 添加详细日志

                    if name == 'app':
                        defaults['cpu0_flash_size'] = parse_size(size_str)
                    elif name == 'app1':
                        defaults['cpu1_flash_size'] = parse_size(size_str)
                    # elif name == 'app2':
                    #     defaults['cpu2_flash_size'] = parse_size(size_str)
                    elif name == 'download':
                        defaults['ota_flash_size'] = parse_size(size_str)
                except KeyError as e:
                    print_error(f"CSV行字段缺失: {e}，完整行内容: {row}")
                    raise

            # 验证必要字段
            # required = ['cpu0_flash_size', 'cpu1_flash_size', 'cpu2_flash_size']
            required = ['cpu0_flash_size', 'cpu1_flash_size']
            missing = [k for k in required if k not in defaults]
            if missing:
                raise ValueError(f"CSV文件缺少必要分区: {', '.join(missing)}")

            print("CSV解析结果:", defaults)
            return defaults

    except Exception as e:
        print_error(f"解析CSV失败，文件路径: {csv_path}，错误详情: {str(e)}")
        raise

def get_current_config(config_path: str) -> Dict[str, Tuple[str, int]]:
    config = {}
    try:
        if not os.path.exists(config_path):
            return config
        with open(config_path, 'r') as f:
            for line in f:
                line = line.strip()
                if match := re.match(r'^(\w+)=(0[xX][0-9a-fA-F]+|[0-9]+[kK]?)$', line):
                    key = match.group(1)
                    value_str = match.group(2)
                    try:
                        parsed_value = parse_size(value_str)
                        config[key] = (value_str, parsed_value)
                    except ValueError:
                        print_warning(f"跳过非数值配置项: {key}={value_str}")
    except Exception as e:
        raise ValueError(f"读取配置文件失败: {str(e)}")
    return config

def load_json_files(file_paths: List[str], flash_defaults: Dict, sram_defaults: Dict, psram_defaults: Dict) -> Tuple[Dict, Dict, Dict, Dict]:
    total_gpio = {}
    sram = {}
    psram = {}
    flash = {}

    for file_path in file_paths:
        try:
            with open(file_path, 'r') as f:
                data = json.load(f)
        except Exception as e:
            raise ValueError(f"解析JSON文件 {file_path} 失败: {str(e)}")

        # 处理SRAM
        if 'sram' in data:
            for key in ['cpu0_sram_size', 'cpu1_sram_size', 'cpu2_sram_size']:
                if key in data['sram']:
                    sram[key] = parse_size(str(data['sram'][key]))
            print(sram)

        # 处理PSRAM
        if 'psram' in data:
            for key in ['cpu0_psram_base', 'cpu0_psram_size', 'cpu1_psram_size']:
                if key in data['psram']:
                    psram[key] = parse_size(str(data['psram'][key]))
            print(psram)

        # 处理Flash
        if 'flash' in data:
            # for key in ['cpu0_flash_size', 'cpu1_flash_size', 'cpu2_flash_size']:
            for key in ['cpu0_flash_size', 'cpu1_flash_size']:
                if key in data['flash']:
                    flash[key] = parse_size(str(data['flash'][key]))
            print(flash)

        # 处理GPIO
        for item in data.get('gpio_map', []):
            gpio_id = item['gpio_id']
            if gpio_id in total_gpio:
                total_gpio[gpio_id].update(item)
            else:
                total_gpio[gpio_id] = item.copy()

    return total_gpio, sram, psram, flash

def generate_header_file(core, items, output_dir):
    field_order = [
        'gpio_id',
        'second_func_en',
        'second_func_dev',
        'io_mode',
        'pull_mode',
        'int_en',
        'int_type',
        'low_power_io_ctrl',
        'driver_capacity'
    ]
    lines = []
    for item in items:
        try:
            values = [item[field] for field in field_order]
        except KeyError as e:
            raise KeyError(f"字段 {e} 在GPIO配置项 {item['gpio_id']} 中缺失")
        line = "    {" + ", ".join(values) + "},\\"
        lines.append(line)

    header_content = f"""#ifndef GPIO_CONFIG_H
#define GPIO_CONFIG_H

#ifdef __cplusplus
extern "C" {{
#endif

#define GPIO_DEFAULT_DEV_CONFIG  \\
{{ \\
{chr(10).join(lines)}
}}

#ifdef __cplusplus
}}
#endif

#endif // GPIO_CONFIG_H
"""
    output_path = os.path.normpath(os.path.join(output_dir, f"usr_gpio_cfg{core}.h"))
    with open(output_path, 'w') as f:
        f.write(header_content)

def process_sram(sram_defaults: Dict, sram_raw: Dict, config_paths: Dict) -> Dict:
    total_sram = 0xA0000  # 655360 bytes

    parsed = {
        'cpu0_sram_size': sram_raw.get('cpu0_sram_size', sram_defaults['cpu0_sram_size']),
        'cpu1_sram_size': sram_raw.get('cpu1_sram_size', sram_defaults['cpu1_sram_size']),
        'cpu2_sram_size': sram_raw.get('cpu2_sram_size', sram_defaults['cpu2_sram_size']),
    }

    # 规则3：cpu2_sram_size必须小于0x10000
    if parsed['cpu2_sram_size'] is not None and parsed['cpu2_sram_size'] >= 0x10000:
        raise ValueError(f"cpu2_sram_size必须小于0x10000: {size_to_hex_str(parsed['cpu2_sram_size'])}")

    # 计算已配置总和
    sum_configured = sum(v for v in parsed.values() if v is not None)
    remaining = total_sram - sum_configured

    if remaining < 0:
        raise ValueError(f"SRAM总和超过0xA0000 (当前总和{sum_configured})")

    # 处理未配置项
    unconfigured = [k for k, v in parsed.items() if v is None]
    if remaining > 0 and unconfigured:
        base = (remaining // len(unconfigured)) // 1024 * 1024
        if base == 0:
            raise ValueError("剩余SRAM空间不足以分配1024对齐的大小")

        for i, key in enumerate(unconfigured):
            # 特殊处理cpu2_sram_size
            if key == 'cpu2_sram_size':
                max_alloc = min(base, 0x10000 - 1)
                if max_alloc <= 0:
                    raise ValueError("无法自动分配符合规则的cpu2_sram_size")
                allocated = max_alloc
            else:
                allocated = base

            allocated = allocated // 1024 * 1024
            parsed[key] = allocated
            remaining -= allocated

    # 验证对齐和总和
    # total = sum(filter(lambda x: x is not None, parsed.values()))
    total = sum(parsed.values())
    for v in parsed.values():
        if v % 1024 != 0:
            raise ValueError(f"SRAM值未1024对齐: {size_to_hex_str(v)}")
    if total != total_sram:
        raise ValueError(f"SRAM总和错误 期望0xA0000 实际0x{total:x}")
    if parsed['cpu2_sram_size'] != None and parsed['cpu2_sram_size'] >= 0x10000:
        raise ValueError(f"cpu2_sram_size必须小于0x10000: {size_to_hex_str(parsed['cpu2_sram_size'])}")

    return parsed

def process_psram(psram_default: Dict, psram_raw: Dict, config_paths: Dict) -> Dict:
    cpu0_config = get_current_config(config_paths['cpu0'])
    cpu1_config = get_current_config(config_paths['cpu1'])

    # 获取默认值（规则10）
    default_cpu0_base = cpu0_config.get('CONFIG_PSRAM_HEAP_BASE', ('0x60700000', 0x60700000))[1]
    default_cpu0_size = cpu0_config.get('CONFIG_PSRAM_HEAP_SIZE', ('0x200000', 0x200000))[1]
    default_cpu1_size = cpu1_config.get('CONFIG_PSRAM_HEAP_SIZE', ('0x700000', 0x700000))[1]

    # 用户配置的值
    user_cpu0_base = psram_raw.get('cpu0_psram_base', default_cpu0_base)
    user_cpu0_size = psram_raw.get('cpu0_psram_size', default_cpu0_size)
    user_cpu1_size = psram_raw.get('cpu1_psram_size')

    # 计算可用空间（规则11）
    max_psram = 0x61000000

    if user_cpu0_base + user_cpu0_size >= max_psram:
        raise ValueError(f"PSRAM 配置错误: {user_cpu0_base} + {user_cpu0_size} >= {max_psram}")

    available_space = max_psram - user_cpu0_base
    print(f'user_cpu0_size:{user_cpu0_size}, default_cpu0_size:{default_cpu0_size}')
    print(f'user_cpu1_size:{user_cpu1_size}, default_cpu1_size:{default_cpu1_size}')

    # 处理cpu0_psram_size和cpu1_psram_size的配置
    if user_cpu1_size is not None:
        # 用户同时配置了cpu0和cpu1的大小，检查总和
        total = user_cpu0_size + user_cpu1_size
        if total != available_space:
            raise ValueError(f"PSRAM总和错误: {size_to_hex_str(total)} != {size_to_hex_str(available_space)}")
    else:
        # 自动计算cpu1_psram_size为剩余空间
        user_cpu1_size = available_space - user_cpu0_size

    # 32K对齐检查（规则9）
    for size in [user_cpu0_size, user_cpu1_size]:
        if size % 32768 != 0:
            raise ValueError(f"PSRAM必须32K对齐: {size_to_hex_str(size)}")

    return {
        'cpu0_psram_base': user_cpu0_base,
        'cpu0_psram_size': user_cpu0_size,
        'cpu1_psram_base': user_cpu0_base + user_cpu0_size,
        'cpu1_psram_size': user_cpu1_size
    }


def process_flash(flash_data: Dict, defaults: Dict) -> Dict:
    total_flash = 6880 * 1024  # 总大小6880K
    # required_keys = ['cpu0_flash_size', 'cpu1_flash_size', 'cpu2_flash_size']
    required_keys = ['cpu0_flash_size', 'cpu1_flash_size']

    # 使用默认值填充未配置项（规则18）
    processed = {k: flash_data.get(k, defaults[k]) for k in required_keys}

    # 检查68K对齐（规则17）
    for key in required_keys:
        if processed[key] % (68*1024) != 0:
            raise ValueError(f"{key}必须为68K倍数: {kbytes_str(processed[key])}")

    # 计算OTA空间（规则16）
    total_used = sum(processed.values())
    ota_size = total_flash - total_used
    if ota_size < 0:
        raise ValueError(f"Flash总空间不足，超出: {-ota_size//1024}K")

    # 验证OTA最小空间（规则20）
    # min_ota = 0.6 * processed['cpu0_flash_size'] + 0.2 * (processed['cpu1_flash_size'] + processed['cpu2_flash_size'])
    min_ota = 0.6 * processed['cpu0_flash_size'] + 0.2 * (processed['cpu1_flash_size'])
    if ota_size < min_ota:
        raise ValueError(f"OTA空间不足: {kbytes_str(ota_size)} < {kbytes_str(min_ota)}")

    processed['ota_flash_size'] = ota_size
    return processed

def update_config(config_path: str, sram: Dict, psram: Dict, cpu: str, sram_defaults: Dict):
    current_config = get_current_config(config_path)
    changes = {}

    # 统一更新所有核心的SRAM配置项（新规则）
    sram_keys = [
        ('CONFIG_CPU0_SPE_RAM_SIZE', 'cpu0_sram_size'),
        ('CONFIG_CPU1_APP_RAM_SIZE', 'cpu1_sram_size'),
        ('CONFIG_CPU2_APP_RAM_SIZE', 'cpu2_sram_size')
    ]

    # 检查并更新所有SRAM配置项
    for config_key, sram_key in sram_keys:
        current_val = current_config.get(config_key, (None, 0))[1]
        new_val = sram[sram_key]

        # 规则23：仅当值变化时更新
        if current_val != new_val:
            changes[config_key] = new_val

        if cpu == '0':
            print(f'sram config: {config_key} {new_val} {current_val}')

    # PSRAM配置更新（规则12-15）
    if cpu in ['0', '1']:
        if cpu == '0':
            base_key = 'CONFIG_PSRAM_HEAP_BASE'
            size_key = 'CONFIG_PSRAM_HEAP_SIZE'
            new_base = psram['cpu0_psram_base']
            new_size = psram['cpu0_psram_size']
        else:
            base_key = 'CONFIG_PSRAM_HEAP_BASE'
            size_key = 'CONFIG_PSRAM_HEAP_SIZE'
            new_base = psram['cpu1_psram_base']
            new_size = psram['cpu1_psram_size']

        current_base = current_config.get(base_key, (None, 0))[1]
        current_size = current_config.get(size_key, (None, 0))[1]
        if current_base != new_base:
            changes[base_key] = new_base
            print(f'cpu{cpu}, psram base: {changes[base_key]} {new_base}')
        if current_size != new_size:
            changes[size_key] = new_size
            print(f'cpu{cpu}, psram size: {changes[size_key]} {new_size}')

        print(f'cpu{cpu}, psram: {current_base} {new_base} {current_size} {new_size}')

    # 应用更改（规则22）
    if changes:
        with open(config_path, 'r+') as f:
            lines = []
            existing_keys = set()
            for line in f:
                line_strip = line.strip()
                key_found = False
                for key in changes:
                    if line_strip.startswith(f"{key}="):
                        hex_val = size_to_hex_str(changes[key])
                        lines.append(f"{key}={hex_val}\n")
                        existing_keys.add(key)
                        key_found = True
                        print(f'cpu{cpu}, {key}')
                        break
                if not key_found and line_strip:
                    lines.append(line)

            # 添加新配置项
            for key in changes:
                if key not in existing_keys:
                    hex_val = size_to_hex_str(changes[key])
                    lines.append(f"{key}={hex_val}\n")
            f.seek(0)
            f.write(''.join(lines))
            f.truncate()

def update_flash_csv(csv_path: str, new_flash: Dict, defaults: Dict):
    if not os.path.exists(csv_path):
        raise FileNotFoundError(f"CSV文件不存在: {csv_path}")

    changed = False
    new_lines = []
    with open(csv_path, 'r') as f:
        reader = csv.reader(f)
        for row in reader:
            name = row[0].strip().lower()
            if name == 'app' and parse_size(row[2]) != new_flash['cpu0_flash_size']:
                row[2] = kbytes_str(new_flash['cpu0_flash_size'])
                changed = True
            elif name == 'app1' and parse_size(row[2]) != new_flash['cpu1_flash_size']:
                row[2] = kbytes_str(new_flash['cpu1_flash_size'])
                changed = True
            # elif name == 'app2' and parse_size(row[2]) != new_flash['cpu2_flash_size']:
            #     row[2] = kbytes_str(new_flash['cpu2_flash_size'])
            #     changed = True
            elif name == 'download' and parse_size(row[2]) != new_flash['ota_flash_size']:
                row[2] = kbytes_str(new_flash['ota_flash_size'])
                changed = True
            new_lines.append(row)

    if changed:
        with open(csv_path, 'w', newline='') as f:
            writer = csv.writer(f)
            writer.writerows(new_lines)
        print_success("Flash分区表已更新")
    else:
        print_info("Flash分区无变化")

def main():
    if len(sys.argv) < 3:
        print("Usage: python script.py <output_dir> <json1> [json2 ...]")
        sys.exit(1)

    print(f"{__file__}: {inspect.currentframe().f_back.f_lineno}")
    output_dir = sys.argv[1]
    json_files = sys.argv[2:]

    try:
        # 初始化路径
        csv_path = os.path.normpath(os.path.join(output_dir, "bk7258/bk7258_partitions.csv"))
        config_paths = {
            'cpu0': os.path.join(output_dir, "bk7258/config"),
            'cpu1': os.path.join(output_dir, "bk7258_cp1/config"),
            'cpu2': os.path.join(output_dir, "bk7258_cp2/config")
        }

        # 加载默认配置
        flash_defaults = load_flash_defaults(csv_path)
        cpu0_config = get_current_config(config_paths['cpu0'])
        cpu1_config = get_current_config(config_paths['cpu1'])
        cpu2_config = get_current_config(config_paths['cpu2'])

        # 初始化默认值
        sram_defaults = {
            'cpu0_sram_size': cpu0_config.get('CONFIG_CPU0_SPE_RAM_SIZE', ('0', 0))[1],
            'cpu1_sram_size': cpu1_config.get('CONFIG_CPU1_APP_RAM_SIZE', ('0', 0))[1],
            'cpu2_sram_size': cpu2_config.get('CONFIG_CPU2_APP_RAM_SIZE', ('0', 0))[1],
        }
        psram_defaults = {
            'cpu0_psram_base': cpu0_config.get('CONFIG_PSRAM_HEAP_BASE', ('0x60700000', 0x60700000))[1],
            'cpu0_psram_size': cpu0_config.get('CONFIG_PSRAM_HEAP_SIZE', ('0x200000', 0x200000))[1],
            'cpu1_psram_size': cpu1_config.get('CONFIG_PSRAM_HEAP_SIZE', ('0x700000', 0x700000))[1],
        }

        # 加载用户配置
        total_gpio, sram_raw, psram_raw, flash_raw = load_json_files(
            json_files, flash_defaults, sram_defaults, psram_defaults
        )

        # 处理各模块配置
        sram = process_sram(sram_defaults, sram_raw, config_paths)
        psram = process_psram(psram_defaults, psram_raw, config_paths)
        flash = process_flash(flash_raw, flash_defaults)

        # 输出结果
        print("\nFinal SRAM Configuration:")
        for k in ['cpu0_sram_size', 'cpu1_sram_size', 'cpu2_sram_size']:
            if sram[k] == None:
                print(f"{k}: 0x00000000")
            else:
                print(f"{k}: {size_to_hex_str(sram[k])}")

        print("\nFinal PSRAM Configuration:")
        print(f"CPU0: 0x{psram['cpu0_psram_base']:x} - {size_to_hex_str(psram['cpu0_psram_size'])}")
        print(f"CPU1: 0x{psram['cpu1_psram_base']:x} - {size_to_hex_str(psram['cpu1_psram_size'])}")

        print("\nFinal Flash Configuration:")
        # for k in ['cpu0_flash_size', 'cpu1_flash_size', 'cpu2_flash_size', 'ota_flash_size']:
        for k in ['cpu0_flash_size', 'cpu1_flash_size', 'ota_flash_size']:
            print(f"{k}: {size_to_hex_str(flash[k])}")

        # 更新配置文件
        update_config(config_paths['cpu0'], sram, psram, '0', sram_defaults)
        update_config(config_paths['cpu1'], sram, psram, '1', sram_defaults)
        update_config(config_paths['cpu2'], sram, psram, '2', sram_defaults)

        # 更新flash.csv
        update_flash_csv(csv_path, flash, flash_defaults)

        # 生成GPIO文件
        sorted_items = sorted(total_gpio.values(), key=lambda x: int(x['gpio_id'].split('_')[1]))
        core_groups = {}
        for item in sorted_items:
            core = item.get('bind_core', '0')
            core_groups.setdefault(core, []).append(item)

        for core, items in core_groups.items():
            generate_header_file(core, items, output_dir)

        print("\nConfiguration files updated successfully")

    except ValueError as e:
        print_error(str(e))
        sys.exit(1)
    except Exception as e:
        print_error(f"Unexpected Error: {str(e)}")
        sys.exit(1)

if __name__ == "__main__":
    main()
