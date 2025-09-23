#include "io_fatfs.h"
#include "debug.h"
#include "ff.h"
#include "mmc.h"
#include "storage.h"
#include "time_date.h"

#if DEBUG_ENABLE
#define LOCAL_DEBUG_LOG_PRINT_ENABLE 0
#endif /* DEBUG_ENABLE */

#if LOCAL_DEBUG_LOG_PRINT_ENABLE
#warning LOCAL_DEBUG_LOG_PRINT_ENABLE
#define LOCAL_DEBUG_LOG_PRINT DEBUG_PRINT_DIRECT_NL
#else /* LOCAL_DEBUG_LOG_PRINT_ENABLE */
#define LOCAL_DEBUG_LOG_PRINT(_f_, ...)
#endif /* LOCAL_DEBUG_LOG_PRINT_ENABLE */

enum {
	FATFS_DRV_EMMC = 0,
	FATFS_DRV_RAM,

	FATFS_DRV_ENUM_SIZE
};

#define DELAY_MAX_CNT 10

static bool emmc_await_delay(const char* pcFuncName) {
	u32 cnt		  = 0;
	u32 cardState = Storage_Emmc_GetCardState();

	while (cardState != HAL_MMC_CARD_TRANSFER) {
		cnt++;
		LOCAL_DEBUG_LOG_PRINT(ESC_COLOR_YELLOW "%s delay: cardState %d" ESC_RESET_STYLE, pcFuncName,
							  cardState);
		PL_DELAY_MS(1);
		cardState = Storage_Emmc_GetCardState();
		if (cnt >= DELAY_MAX_CNT) {
			LOCAL_DEBUG_LOG_PRINT(ESC_COLOR_RED "await failed" ESC_RESET_STYLE);
			return false;
		}
	}

	return true;
}

DSTATUS disk_initialize(BYTE pdrv) {
	DSTATUS stat = STA_NOINIT;

	switch (pdrv) {
		case FATFS_DRV_EMMC:
			stat = STA_OK;
			break;

		case FATFS_DRV_RAM:
			stat = STA_OK;
			break;
	}

	// LOCAL_DEBUG_LOG_PRINT("disk_initialize: drv %d, res %d, cardState %d", pdrv, stat,
	// 					  Storage_Emmc_GetCardState());
	// ASSERT_CHECK(stat == STA_OK);
	return stat;
}

DSTATUS disk_status(BYTE pdrv) {
	DSTATUS stat = STA_NOINIT;

	switch (pdrv) {
		case FATFS_DRV_EMMC:
			if (emmc_await_delay(__FUNCTION__))
				stat = STA_OK;
			break;

		case FATFS_DRV_RAM:
			stat = STA_OK;
			break;
	}

	LOCAL_DEBUG_LOG_PRINT("disk_status: drv %d, res %d", pdrv, stat);
	// ASSERT_CHECK(stat == STA_OK);
	return stat;
}

DRESULT disk_read(BYTE pdrv, BYTE* pBuff, LBA_t sector, UINT count) {
	DRESULT res = RES_ERROR;

	switch (pdrv) {
		case FATFS_DRV_EMMC:
			//TODO add speed test, add DMA handling, check interrupts

			if (emmc_await_delay(__FUNCTION__))
				res = Storage_Emmc_Read(pBuff, sector, count) == true ? RES_OK : RES_ERROR;
			break;

		case FATFS_DRV_RAM:
			Storage_Ram_Read(pBuff, sector * FF_MAX_SS, count * FF_MAX_SS);
			res = RES_OK;
			break;
	}

	LOCAL_DEBUG_LOG_PRINT("disk_read: drv %d, res %d, sector %d, count %d", pdrv, res, sector,
						  count);
	// ASSERT_CHECK(res == RES_OK);
	return res;
}

#if FF_FS_READONLY == 0

DRESULT disk_write(BYTE pdrv, const BYTE* pBuff, LBA_t sector, UINT count) {
	DRESULT res = RES_ERROR;

	switch (pdrv) {
		case FATFS_DRV_EMMC:

			if (emmc_await_delay(__FUNCTION__))
				res = Storage_Emmc_Write(pBuff, sector, count) == true ? RES_OK : RES_ERROR;
			break;

		case FATFS_DRV_RAM:
			Storage_Ram_Write(pBuff, sector * FF_MAX_SS, count * FF_MAX_SS);
			res = RES_OK;
			break;
	}

	LOCAL_DEBUG_LOG_PRINT("disk_write: drv %d, res %d, sector %d, count %d", pdrv, res, sector,
						  count);
	// ASSERT_CHECK(res == RES_OK);
	return res;
}

#endif /*  FF_FS_READONLY == 0 */

DRESULT disk_ioctl(BYTE pdrv, BYTE cmd, void* pBuff) {
	DRESULT res = RES_ERROR;

	switch (pdrv) {
		case FATFS_DRV_EMMC: {
			LOCAL_DEBUG_LOG_PRINT("disk_ioctl: cardState %d", Storage_Emmc_GetCardState());

			Pl_SdEmmcInfo_t sdEmmcInfo;
			if (Storage_Emmc_GetDeviceInfo(&sdEmmcInfo) != true)
				return RES_ERROR;

			switch (cmd) {
				case CTRL_SYNC:
					res = RES_OK;
					break;
				case GET_SECTOR_COUNT:
					*(DWORD*)pBuff = sdEmmcInfo.LogBlockNbr;
					res			   = RES_OK;
					break;
				case GET_SECTOR_SIZE:
					*(WORD*)pBuff = sdEmmcInfo.LogBlockSize;
					res			  = RES_OK;
					break;
				case GET_BLOCK_SIZE:
					*(DWORD*)pBuff = sdEmmcInfo.LogBlockSize / sdEmmcInfo.BlockSize;
					res			   = RES_OK;
					break;
				default:
					res = RES_PARERR;
			}
			break;
		} break;

		case FATFS_DRV_RAM: {
			switch (cmd) {
				case CTRL_SYNC:
					res = RES_OK;
					break;
				case GET_SECTOR_COUNT:
					*(DWORD*)pBuff = Storage_Ram_GetStorageCap() / FF_MAX_SS;
					res			   = RES_OK;
					break;
				case GET_SECTOR_SIZE:
					*(WORD*)pBuff = FF_MAX_SS;
					res			  = RES_OK;
					break;
				case GET_BLOCK_SIZE:
					*(DWORD*)pBuff = 1;
					res			   = RES_OK;
					break;
				default:
					res = RES_PARERR;
			}
			break;
		} break;
	}

	LOCAL_DEBUG_LOG_PRINT("disk_ioctl: drv %d, res %d, cmd %d, buff %d", pdrv, res, cmd,
						  ((u32*)pBuff)[0]);
	// ASSERT_CHECK(res == RES_OK);
	return res;
}

DWORD get_fattime(void) {
	if (!TimeDate_IsReady())
		return 0;

	TimeDate_t rtc;
	TimeDate_Struct_Init(&rtc);
	TimeDate_TimeDate_Get(&rtc);

	DWORD res = (((DWORD)rtc.Year - 1980) << 25) | ((DWORD)rtc.Month << 21) |
				((DWORD)rtc.Day << 16) | (WORD)(rtc.Hour << 11) | (WORD)(rtc.Minute << 5) |
				(WORD)(rtc.Second >> 1);

	return res;
}
