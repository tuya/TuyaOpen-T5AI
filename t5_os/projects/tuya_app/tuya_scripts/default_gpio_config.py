import json
import argparse
import os

def generate_h_file(json_file, output_path):
    # 读取 JSON 文件
    with open(json_file) as file:
        data = json.load(file)

    gpio_map = data['gpio_map']

    # C 头文件生成
    h_code = """
#ifndef GPIO_CONFIG_H
#define GPIO_CONFIG_H

#ifdef __cplusplus
extern "C" {
#endif

#define GPIO_DEFAULT_DEV_CONFIG  \\
{ \\
"""

    # 遍历 GPIO 配置，生成结构体初始化
    for i, gpio in enumerate(gpio_map):
        h_code += f"    {{{gpio['gpio_id']}, {gpio['second_func_en']}, {gpio['second_func_dev']}, {gpio['io_mode']}, {gpio['pull_mode']}, {gpio['int_en']}, {gpio['int_type']}, {gpio['low_power_io_ctrl']}, {gpio['driver_capacity']}}}"

        # 如果不是最后一个元素，添加逗号
        if i < len(gpio_map) - 1:
            h_code += ",\\\n"
        else:
            h_code += "\\\n"  # 最后一个元素后不加逗号

    # 结束定义
    h_code += "}\n"

    h_code += "#endif // GPIO_CONFIG_H\n"

    h_code += "#ifdef __cplusplus\n"
    h_code += "}\n"
    h_code += "#endif\n"


    # 生成的 C 文件名
    h_file_name = os.path.join(output_path, "usr_gpio_cfg.h")

    # 写入头文件
    with open(h_file_name, 'w') as h_file:
        h_file.write(h_code)

    print(f"H file has been generated as '{h_file_name}'")

def main():
    # 设置命令行参数解析
    parser = argparse.ArgumentParser(description='Generate a C header file with GPIO configuration from a JSON file.')
    parser.add_argument('json_file', type=str, help='Path to the JSON file')
    parser.add_argument('output_path', type=str, help='Path to save the generated header file')

    args = parser.parse_args()

    # 调用生成 H 文件的函数
    generate_h_file(args.json_file, args.output_path)

if __name__ == "__main__":
    main()
