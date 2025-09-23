#ifndef __STORAGE_UTILS_H
#define __STORAGE_UTILS_H

#include "main.h"

#define FS_TEST_SPEED_WRITE_MIN (0.25f)
#define FS_TEST_SPEED_READ_MIN	(2.0f)
#define FS_TEST_BUFF_SIZE		(PL_SDMMC_SECTOR_SIZE * 4)
#define FS_TEST_FILE_SIZE		(FS_TEST_BUFF_SIZE * 40)
#define FS_TEST_FILE_NAME_LEN	(64)

RET_STATE_t StorageUtils_FsSpeedTest(const char* pPath, float* pReadSpeed, float* pWriteSpeed);

#endif /* __STORAGE_UTILS_H */
