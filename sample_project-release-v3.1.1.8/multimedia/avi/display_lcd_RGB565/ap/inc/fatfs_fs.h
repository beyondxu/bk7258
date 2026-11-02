#ifndef __FATFS_FS_H__
#define __FATFS_FS_H__

/**
 * @brief 挂载 SD 卡上的 FATFS 文件系统
 * @return 0 成功，其他值表示错误码
 */
int sdcard_fatfs_mount();

/**
 * @brief 卸载 SD 卡上的 FATFS 文件系统
 * @return 0 成功，其他值表示错误码
 */
int sdcard_fatfs_umount();

#endif