#include "os/os.h"
#include "bk_posix.h"
#include "driver/flash_partition.h"
#include "sdkconfig.h"
#include <driver/qspi_flash_common.h>
#include "tkl_system.h"

#include "driver/sd_card_types.h"

int fatfs_mount(const char *mount_path, int type)
{
    int ret;
    sd_card_info_t card_info;
    const char *fs_type = "fatfs";
    struct bk_fatfs_partition partition;

    if (mount_path == NULL) {
        bk_printf("fatfs mount failed, no mount point\r\n");
        return -1;
    }

    bk_sd_card_init();

    bk_sd_card_get_card_info(&card_info);

    uint32_t cap = bk_sd_card_get_card_size();
    sd_card_state_t state = bk_sd_card_get_card_state();

    bk_printf("mount on sdcard, version: %d, type: %d, class: %d, rca: %d, size: %d, stat: %d\r\n",
            card_info.card_version, card_info.card_type, card_info.class,
            card_info.relative_card_addr, cap, state);

    partition.part_type = FATFS_DEVICE;
    partition.part_dev.device_name = FATFS_DEV_SDCARD;  // f_mount需求该参数，绑定底层硬件接口
    partition.mount_path = mount_path;

    // fs_type: fatfs / littlefs
    ret = mount("SOURCE_NONE", partition.mount_path, fs_type, 0, &partition);

    bk_printf("mount fatfs, %s %d\r\n", partition.mount_path, ret);

    return ret;
}
//#endif // CONFIG_FATFS

#if CONFIG_LITTLEFS
int littlefs_mount(const char *mount_path, int type)
{
#define QSPI_FLASH_ADDR    0

    int ret, retry = 0;
    struct bk_little_fs_partition partition;
    const char *fs_type = "littlefs";

    if (mount_path == NULL) {
        bk_printf("lfs mount failed, no mount point\r\n");
        return -1;
    }

    if (type == LFS_QSPI_FLASH) {
        qspi_driver_desc_t *qflash_dev = NULL;

        qflash_dev = tuya_qspi_device_query(CONFIG_TUYA_QSPI_FLASH_TYPE);
        if (qflash_dev == NULL) {
            bk_printf("Not found qspi flash %s\r\n", CONFIG_TUYA_QSPI_FLASH_TYPE);
            return BK_FAIL;
        }

        bk_printf("mkfs on qspi flash, total size: %d, block size: %d\r\n",
                qflash_dev->total_size, qflash_dev->block_size);

        partition.part_type = LFS_QSPI_FLASH;
        partition.part_flash.start_addr = QSPI_FLASH_ADDR;
        partition.part_flash.size = qflash_dev->total_size;
        partition.part_flash.page_size = qflash_dev->page_size;
        partition.part_flash.block_size = qflash_dev->block_size;
    } else if (type == LFS_FLASH) {
        bk_logic_partition_t *pt = bk_flash_partition_get_info(BK_PARTITION_USR_CONFIG);

        partition.part_type = LFS_FLASH;
        partition.part_flash.start_addr = pt->partition_start_addr;
        partition.part_flash.size = pt->partition_length;
        // TODO: 片内flash，page size一般为256字节，
        // block size一般为4096, 如果flash参数不同，该处需要修改
        partition.part_flash.page_size = 256;
        partition.part_flash.block_size = 4096;
    } else {
        bk_printf("device type not support: %d\r\n", type);
        return -1;
    }
    partition.mount_path = mount_path;

    do {
        ret = mount("SOURCE_NONE", partition.mount_path, fs_type, 0, &partition);
        if (ret == BK_OK) {
            bk_printf("mount littlefs %s ok\r\n", partition.mount_path);
            break;
        }
        retry++;
        tkl_system_sleep(50);
    } while ((ret != BK_OK) && (retry < 3));

#if 0
    if ((ret != BK_OK) && (retry >= 3)) {
        bk_printf("mount littlefs %s failed, begin to mkfs\r\n", partition.mount_path);
        ret = mkfs("PART_NONE", fs_type, &partition);
        if (ret == BK_OK) {
            bk_printf("mount littlefs, %s failed begin to mkfs success\r\n", partition.mount_path);
            ret = mount("SOURCE_NONE", partition.mount_path, fs_type, 0, &partition);
            if (ret  != BK_OK) {
                bk_printf("re-mount littlefs, %s failed begin to mkfs failed\r\n", partition.mount_path);
            }
        }
    }
#endif

    return ret;
}
#endif // CONFIG_LITTLEFS

