#! /usr/bin/env python3
# vim:fenc=utf-8
#
# Copyright © 2025 cc <cc@tuya>
#
# Distributed under terms of the MIT license.


import json
import os

def load_json_files(file_paths):
    total_gpio = {}
    for file_path in file_paths:
        print("file:{}".format(file_path))
        with open(file_path, 'r') as f:
            data = json.load(f)
            for item in data['gpio_map']:
                gpio_id = item['gpio_id']
                if gpio_id in total_gpio:
                    # 更新现有项，仅覆盖提供的字段
                    total_gpio[gpio_id].update(item)
                else:
                    # 新项，直接添加
                    total_gpio[gpio_id] = item.copy()
    return total_gpio

def sort_gpio_items(gpio_dict):
    items = list(gpio_dict.values())
    items.sort(key=lambda x: int(x['gpio_id'].split('_')[1]))
    return items

def group_by_core(sorted_items):
    core_groups = {}
    for item in sorted_items:
        core = item['bind_core']
        if core not in core_groups:
            core_groups[core] = []
        core_groups[core].append(item)
    return core_groups

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
            raise KeyError(f"字段 {e} 在GPIO配置项 {item['gpio_id']} 中缺失，请确保所有配置项完整。")
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
    output_path = os.path.join(output_dir, f"usr_gpio_cfg{core}.h")
    with open(output_path, 'w') as f:
        f.write(header_content)

def main():
    import sys
    if len(sys.argv) < 3:
        print("用法: python script.py <输出目录> <JSON文件1> <JSON文件2> ...")
        sys.exit(1)

    output_dir = sys.argv[1]
    json_files = sys.argv[2:]

    try:
        total_gpio = load_json_files(json_files)
        sorted_items = sort_gpio_items(total_gpio)
        core_groups = group_by_core(sorted_items)

        os.makedirs(output_dir, exist_ok=True)

        for core, items in core_groups.items():
            generate_header_file(core, items, output_dir)
        print(f"成功生成配置文件到目录: {output_dir}")
    except Exception as e:
        print(f"错误发生: {str(e)}")
        sys.exit(1)

if __name__ == "__main__":
    main()
