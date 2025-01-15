#!/usr/bin/env python3

import io
import os
import struct

class Image:
    FILE_HEAD_MAGIC0 = "BKDLV10."
    FILE_HEAD_MAGIC1 = "BL1DLV10"
    FILE_OTA_HEAD_MAGIC = "BK723658"

    def __init__(self, file_path):
        self.file_path = file_path
        self.file_magic = None
        self.chip_type = None
        self.global_header = {}
        self.sub_images = []

    def get_file_size(self, file_path):
        file_size = 0
        with open(file_path, "rb") as f:
            f.seek(0, os.SEEK_END)
            file_size = f.tell()
        return file_size

    def parse_all_image(self, file_path):
        with open(self.file_path, 'rb') as f:
            # parsing globle header
            self.global_header['magic'] = f.read(8)
            self.global_header['crc'] = struct.unpack('>I', f.read(4))[0]
            self.global_header['version'] = struct.unpack('>I', f.read(4))[0]
            self.global_header['hdr_len'] = struct.unpack('>H', f.read(2))[0]
            self.global_header['img_num'] = struct.unpack('>H', f.read(2))[0]
            self.global_header['flags'] = struct.unpack('>I', f.read(4))[0]
            self.global_header['reserved'] = f.read(8)

            print(f"version: {self.global_header['version']}")
            print(f"img_num: {self.global_header['img_num']}")
             # Parsing each sub-image header
            for n in range(self.global_header['img_num']):
                sub_image = {}
                sub_image['partition_start'] = struct.unpack('>I', f.read(4))[0]
                sub_image['partition_size'] = struct.unpack('>I', f.read(4))[0]
                sub_image['flash_start_addr'] = struct.unpack('>I', f.read(4))[0]
                sub_image['img_offset'] = struct.unpack('>I', f.read(4))[0]
                sub_image['img_len'] = struct.unpack('>I', f.read(4))[0]
                sub_image['checksum'] = struct.unpack('>I', f.read(4))[0]
                sub_image['version'] = struct.unpack('>I', f.read(4))[0]
                sub_image['type'] = struct.unpack('>H', f.read(2))[0]
                sub_image['reserved'] = struct.unpack('>H', f.read(2))[0]
                self.sub_images.append(sub_image)

                if n == 0:
                    f.seek(0x190)
                    image_type = f.read(6)
                    if image_type[:2] == bytes([ord(char) for char in "BK"]):
                        image_type_str = image_type.decode()
                        print(f"image_type: {image_type_str}")
                    else:
                        print("image_type: unknow")
                    print()

                print(f"image: {n}")
                print("  partition_start: 0x{:08x}".format(sub_image['partition_start']))
                print("  partition_size: 0x{:08x}".format(sub_image['partition_size']))
                print("  flash_start_addr: 0x{:08x}".format(sub_image['flash_start_addr']))
                print("  img_offset: 0x{:08x}".format(sub_image['img_offset']))
                print("  img_len: 0x{:08x}".format(sub_image['img_len']))
                print("  type: {}".format(sub_image['type']))
                print()

    def parse_ota_image(self, file_path):
        ota_global_hdr_len = 32
        ota_image_hdr_len = 32
        sign_version_offset = 20
        with open(self.file_path, 'rb') as f:
            # parsing globle header
            self.global_header['magic'] = f.read(8)
            self.global_header['crc'] = struct.unpack('<I', f.read(4))[0]
            self.global_header['version'] = struct.unpack('<I', f.read(4))[0]
            self.global_header['hdr_len'] = struct.unpack('<H', f.read(2))[0]
            self.global_header['img_num'] = struct.unpack('<H', f.read(2))[0]
            self.global_header['flags'] = struct.unpack('<I', f.read(4))[0]
            self.global_header['reserved'] = f.read(8)

            print(f"version: {self.global_header['version']}")
            print(f"img_num: {self.global_header['img_num']}")
             # Parsing each sub-image header
            for n in range(self.global_header['img_num']):
                sub_image = {}
                sub_image['img_len'] = struct.unpack('<I', f.read(4))[0]
                sub_image['img_offset'] = struct.unpack('<I', f.read(4))[0]
                sub_image['flash_offset'] = struct.unpack('<I', f.read(4))[0]
                sub_image['checksum'] = struct.unpack('<I', f.read(4))[0]
                sub_image['version'] = struct.unpack('<I', f.read(4))[0]
                sub_image['flags'] = struct.unpack('<I', f.read(4))[0]
                sub_image['reserved'] = f.read(8)
                self.sub_images.append(sub_image)

                print(f"image: {n}")
                print("  img_len: 0x{:08x}".format(sub_image['img_len']))
                print("  img_offset: 0x{:08x}".format(sub_image['img_offset']))
                print("  flash_offset: 0x{:08x}".format(sub_image['flash_offset']))
                print("  flags: 0x{:08x}".format(sub_image['flags']))
                print()

            f.seek(ota_global_hdr_len + (n+1) * ota_image_hdr_len + sign_version_offset)
            iv_major = struct.unpack('B', f.read(1))[0]
            iv_minor = struct.unpack('B', f.read(1))[0]
            iv_revision = struct.unpack('<H', f.read(2))[0]
            print(f'app_version:{iv_major}.{iv_minor}.{iv_revision}\r\n')

    def parse_normal_image(self, file_path):
        with open(self.file_path, 'rb') as f:
            # parsing globle header
            msp = struct.unpack('<I', f.read(4))[0]
            entry = struct.unpack('<I', f.read(4))[0]
            print("msp: 0x{:08x}".format(msp))
            print("entry:  0x{:08x}".format(entry))
            f.seek(0x100)
            image_type = f.read(6)

            if image_type[:2] == bytes([ord(char) for char in "BK"]):
                image_type_str = image_type.decode()
                print(f"image_type: {image_type_str}")
            else:
                print("image_type: unknow")
            print()

    def image_info(self):
        print(f"File size: {self.get_file_size(self.file_path)} (bytes)")
        with open(self.file_path, "rb") as f:
            try:
                magic = f.read(8)
            except IndexError:
                raise FatalError("File is empty")

       # magic_str = magic.decode('utf-8')
        MAGIC0 = bytes([ord(char) for char in self.FILE_HEAD_MAGIC0])
        MAGIC1 = bytes([ord(char) for char in self.FILE_HEAD_MAGIC1])
        OTA_MAGIC = bytes([ord(char) for char in self.FILE_OTA_HEAD_MAGIC])

        if magic == MAGIC0 or magic == MAGIC1:
            self.parse_all_image(self.file_path)
        elif magic == OTA_MAGIC:
            self.parse_ota_image(self.file_path)
        else:
            self.parse_normal_image(self.file_path)

def mage_info_processing(loadfile):
    image = Image(loadfile)
    image.image_info()
