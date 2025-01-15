#! /usr/bin/env python3
# vim:fenc=utf-8
#
# Copyright © 2024 cc <cc@tuya>
#
# Distributed under terms of the MIT license.

"""

"""
import os
import csv
import argparse

class Part_Item:
    # 类属性：存储所有的实例
    instances = []

    def __init__(self, name, start, length):
        self.name = name
        self.start = start
        self.length = length
        # 将每个新创建的实例添加到类属性中
        Part_Item.instances.append(self)

    def __repr__(self):
        return f"Part_Item(name='{self.name}', start address='{self.start}', length={self.length})"

    @classmethod
    def find_partition(cls, name=None):
        if name is None:
            print("spec partition name")
            return None

        for item in cls.instances:
            if name.lower() in item.name.lower():
                return item

        return None

# 解析 CSV 文件并返回大小
def convert_to_bytes(length_str):
    """将大小字符串转换为字节数"""
    if length_str.endswith('k') or length_str.endswith('K'):
        return (int(length_str[:-1]) * 1024)  # kB 转换为字节
    elif length_str.endswith('M') or length_str.endswith('m'):
        return int(length_str[:-1]) * 1024 * 1024  # MB 转换为字节
    elif length_str.endswith('G') or length_str.endswith('g'):
        return int(length_str[:-1]) * 1024 * 1024 * 1024  # GB 转换为字节
    return int(length_str)  # 直接返回整数

def parse_file(file):
    last_end_address = 0
    with open(file, newline='', encoding='utf-8') as csvfile:
        csvreader = csv.reader(csvfile)

        # 逐行读取数据
        for row in csvreader:
            # 忽略以#开头的行
            if row[0].startswith('#'):
                continue

            name = row[0]
            start_address = row[1]
            length = row[2]

            # 计算当前项的大小（字节）
            length_bytes = convert_to_bytes(length) if length else 0
            current_start_address = start_address if start_address else hex(last_end_address)  # 使用结束地址或空值

            # 记录项的信息
            Part_Item(name, current_start_address, int((length_bytes / 34) * 32))

            # 更新结束地址
            last_end_address += length_bytes

        print(Part_Item)

def create_padded_file(bin0_path, bin1_path, bin2_path, output_path, total_length=4 * 1024 * 1024):

    app0 = Part_Item.find_partition('app')
    app1 = Part_Item.find_partition('app1')
    if bin2_path is not None:
        app2 = Part_Item.find_partition('app2')

    print(app0)
    print(app1)
    if bin2_path is not None:
        print(app2)

    # 获取每个文件的大小
    length_0 = os.path.getsize(bin0_path)
    length_1 = os.path.getsize(bin1_path)
    if bin2_path is not None:
        length_2 = os.path.getsize(bin2_path)


    # 计算填充的大小
    padding_0_length = app0.length - length_0
    if bin2_path is not None:
        padding_1_length = app1.length - length_1
    else:
        padding_1_length = 0

    print("cpu0 size {}, partition size {}, padding {}".format(length_0, app0.length, padding_0_length))
    print("cpu1 size {}, partition size {}, padding {}".format(length_1, app1.length, padding_1_length))
    if bin2_path is not None:
        print("cpu2 size {}, partition size {}".format(length_2, app2.length))

    with open(output_path, 'wb') as output_file:
        # 读取第一个二进制文件并写入输出文件
        with open(bin0_path, 'rb') as bin1_file:
            output_file.write(bin1_file.read(length_0))

        # 写入填充
        output_file.write(bytes([0xFF] * padding_0_length))

        # 读取第二个二进制文件并写入输出文件
        with open(bin1_path, 'rb') as bin2_file:
            output_file.write(bin2_file.read(length_1))

        if bin2_path is not None:
            # 写入填充
            output_file.write(bytes([0xFF] * padding_1_length))

            # 读取第三个二进制文件并写入输出文件
            with open(bin2_path, 'rb') as bin3_file:
                output_file.write(bin3_file.read(length_2))

if __name__ == "__main__":

    parser = argparse.ArgumentParser(description='handle arguments')
    parser.add_argument('partition_file', type=str, help='partition file')
    parser.add_argument('cpu0_bin', type=str, help='cpu0 bin file')
    parser.add_argument('cpu1_bin', type=str, help='cpu1 bin file')
    parser.add_argument('--cpu2_bin', type=str, help='cpu2 bin file')
    parser.add_argument('--ua_file', type=str, default='ua.bin', help='ua path')

    args = parser.parse_args()
    print(f"partition_file: {args.partition_file}")

    parse_file(args.partition_file)

    print(Part_Item)

    print(args.ua_file)

    create_padded_file(args.cpu0_bin, args.cpu1_bin, args.cpu2_bin, args.ua_file)



