#include "io_msc.h"
#include "debug.h"
#include "mmc.h"
#include "storage.h"
#include "usb.h"
#include "usbd_msc.h"

#if DEBUG_ENABLE
#define LOCAL_DEBUG_LOG_PRINT_ENABLE 0
#endif /* DEBUG_ENABLE */

#if LOCAL_DEBUG_LOG_PRINT_ENABLE
#warning LOCAL_DEBUG_LOG_PRINT_ENABLE
#define LOCAL_DEBUG_LOG_PRINT DEBUG_PRINT_DIRECT_NL
#else /* LOCAL_DEBUG_LOG_PRINT_ENABLE */
#define LOCAL_DEBUG_LOG_PRINT(_f_, ...)
#endif /* LOCAL_DEBUG_LOG_PRINT_ENABLE */

typedef s8 (*USB_MSC_Init_FS_Func)(u8 lun);
typedef s8 (*USB_MSC_GetCapacity_FS_Func)(u8 lun, u32* pBlockNum, u16* pBlockSize);
typedef s8 (*USB_MSC_IsReady_FS_Func)(u8 lun);
typedef s8 (*USB_MSC_IsWriteProtected_FS_Func)(u8 lun);
typedef s8 (*USB_MSC_Read_FS_Func)(u8 lun, u8* pBuff, u32 blockAddr, u16 blockLen);
typedef s8 (*USB_MSC_Write_FS_Func)(u8 lun, u8* pBuff, u32 blockAddr, u16 blockLen);
typedef s8 (*USB_MSC_GetMaxLun_FS_Func)(void);

enum {
	MSC_LUN_RAM	 = 0,
	MSC_LUN_EMMC = 1,

	MSC_LUN_ENUM_SIZE = 2
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

static u8 USB_MSC_Init(u8 lun) {
	u8 retState = USBD_FAIL;

	switch (lun) {
		case MSC_LUN_EMMC:
			retState = USBD_OK;
			break;

		case MSC_LUN_RAM:
			retState = USBD_OK;
			break;
	}

	// LOCAL_DEBUG_LOG_PRINT("USB_MSC_Init: lun %d, stat %d", lun, retState);
	// ASSERT_CHECK(retState == USBD_OK);
	return retState;
}

static u8 USB_MSC_IsReady(u8 lun) {
	u8 retState = USBD_FAIL;

	switch (lun) {
		case MSC_LUN_EMMC:
			if (emmc_await_delay(__FUNCTION__))
				retState = USBD_OK;
			else
				retState = USBD_BUSY;
			break;

		case MSC_LUN_RAM:
			retState = USBD_OK;
			break;
	}

	LOCAL_DEBUG_LOG_PRINT("USB_MSC_IsReady: lun %d, res %d", lun, retState);
	// ASSERT_CHECK(retState == USBD_OK || retState == USBD_BUSY);
	return retState;
}

static u8 USB_MSC_IsWriteProtected(u8 lun) {
	u8 retState = USBD_FAIL;

	switch (lun) {
		case MSC_LUN_EMMC:
			retState = USBD_OK;
			break;
		case MSC_LUN_RAM:
			retState = USBD_OK;
			break;
	}

	LOCAL_DEBUG_LOG_PRINT("USB_MSC_IsWriteProtected: lun %d, stat %d", lun, retState);
	// ASSERT_CHECK(retState == USBD_OK);
	return retState;
}

static u8 USB_MSC_Read(u8 lun, u8* pBuff, u32 blockAddr, u16 blockLen) {
	u8 retState = USBD_FAIL;

	switch (lun) {
		case MSC_LUN_EMMC:
			if (emmc_await_delay(__FUNCTION__))
				retState =
					Storage_Emmc_Read(pBuff, blockAddr, blockLen) == true ? USBD_OK : USBD_BUSY;
			break;

		case MSC_LUN_RAM:
			Storage_Ram_Read(pBuff, blockAddr * APP_RAM_MSC_BLOCK_SIZE,
							 blockLen * APP_RAM_MSC_BLOCK_SIZE);
			retState = USBD_OK;
			break;
	}

	LOCAL_DEBUG_LOG_PRINT("USB_MSC_Read: lun %d, res %d, from block %d with len %d", lun, retState,
						  blockAddr, blockLen);
	// ASSERT_CHECK(retState == USBD_OK || retState == USBD_BUSY);
	return retState;
}

static u8 USB_MSC_Write(u8 lun, u8* pBuff, u32 blockAddr, u16 blockLen) {
	u8 retState = USBD_FAIL;

	switch (lun) {
		case MSC_LUN_EMMC:
			if (emmc_await_delay(__FUNCTION__))
				retState =
					Storage_Emmc_Write(pBuff, blockAddr, blockLen) == true ? USBD_OK : USBD_BUSY;
			break;

		case MSC_LUN_RAM:
			Storage_Ram_Write(pBuff, blockAddr * APP_RAM_MSC_BLOCK_SIZE,
							  blockLen * APP_RAM_MSC_BLOCK_SIZE);
			retState = USBD_OK;
			break;
	}

	LOCAL_DEBUG_LOG_PRINT("USB_MSC_Write: lun %d, res %d, to block %d with len %d", lun, retState,
						  blockAddr, blockLen);
	// ASSERT_CHECK(retState == USBD_OK || retState == USBD_BUSY);
	return retState;
}

static u8 USB_MSC_GetCapacity(u8 lun, u32* pBlockNum, u16* pBlockSize) {
	u8 retState = USBD_FAIL;

	switch (lun) {
		case MSC_LUN_EMMC: {
			LOCAL_DEBUG_LOG_PRINT("USB_MSC_GetCapacity: cardState %d", Storage_Emmc_GetCardState());

			Pl_SdEmmcInfo_t sdEmmcInfo;
			if (Storage_Emmc_GetDeviceInfo(&sdEmmcInfo) != true)
				return USBD_FAIL;

			*pBlockNum	= sdEmmcInfo.LogBlockNbr;
			*pBlockSize = sdEmmcInfo.LogBlockSize;
			retState	= USBD_OK;
		} break;

		case MSC_LUN_RAM: {
			*pBlockNum	= Storage_Ram_GetStorageCap() / APP_RAM_MSC_BLOCK_SIZE;
			*pBlockSize = APP_RAM_MSC_BLOCK_SIZE;
			retState	= USBD_OK;
		} break;
	}

	LOCAL_DEBUG_LOG_PRINT("USB_MSC_GetCapacity: lun %d, res %d, blocks %d, size %d", lun, retState,
						  *pBlockNum, *pBlockSize);
	// ASSERT_CHECK(retState == USBD_OK);
	return retState;
}

static u8 USB_MSC_GetMaxLun(void) {
	LOCAL_DEBUG_LOG_PRINT("USB_MSC_GetMaxLun");
	return MSC_LUN_ENUM_SIZE - 1;
}

// clang-format off
const s8 USB_MSC_Inquirydata_FS[] = { // 36 + 36
	/* LUN 0 */
	0x00, // bit4:0 - peripheral device type, bit7:5 - reserved
	0x80, // bit6:0 - reserved, bit7 - removable media bit (set to 1 to indicate removable media) 
	0x02, // bit2:0 - ANSI version, bit5:3 - ECMA version, bit7:6 - ISO version
	0x02, // bit3:0 - Response data format, bit7:4 - reserved
	(STANDARD_INQUIRY_DATA_LEN - 5), // additional length - specify the length in bytes of the parameters.
	0x00, // reserved
	0x00, // reserved
	0x00, // reserved
	'P', 'R', 'S', 'S', ' ', ' ', ' ', ' ',	/* Manufacturer : 8 bytes */
	'R', 'O', 'M', ' ', 'D', 'i', 's', 'c',	/* Product      : 16 Bytes */
	' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ',
	'0', '.', '0', '1',						/* Version      : 4 Bytes */

	/* LUN 1 */
	0x00, // bit4:0 - peripheral device type, bit7:5 - reserved
	0x80, // bit6:0 - reserved, bit7 - removable media bit (set to 1 to indicate removable media)
	0x02, // bit2:0 - ANSI version, bit5:3 - ECMA version, bit7:6 - ISO version
	0x02, // bit3:0 - Response data format, bit7:4 - reserved
	(STANDARD_INQUIRY_DATA_LEN - 5), // additional length - specify the length in bytes of the parameters.
	0x00, // reserved
	0x00, // reserved
	0x00, // reserved
	'P', 'R', 'S', 'S', ' ', ' ', ' ', ' ',	/* Manufacturer : 8 bytes */
	'R', 'A', 'M', ' ', 'D', 'i', 's', 'k',	/* Product      : 16 Bytes */
	' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ',
	'0', '.', '0', '1'						/* Version      : 4 Bytes */
};
// clang-format on

USBD_StorageTypeDef USBD_StorageInterface = {
	(USB_MSC_Init_FS_Func)USB_MSC_Init,
	(USB_MSC_GetCapacity_FS_Func)USB_MSC_GetCapacity,
	(USB_MSC_IsReady_FS_Func)USB_MSC_IsReady,
	(USB_MSC_IsWriteProtected_FS_Func)USB_MSC_IsWriteProtected,
	(USB_MSC_Read_FS_Func)USB_MSC_Read,
	(USB_MSC_Write_FS_Func)USB_MSC_Write,
	(USB_MSC_GetMaxLun_FS_Func)USB_MSC_GetMaxLun,
	(s8*)USB_MSC_Inquirydata_FS,
};
