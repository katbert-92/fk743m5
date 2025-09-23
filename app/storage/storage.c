#include "storage.h"
#include "debug.h"
#include "ff.h"
#include "fs_wrapper.h"
#include "rtos_analyzer.h"

#if DEBUG_ENABLE
#define LOCAL_DEBUG_PRINT_ENABLE 0
#define LOCAL_DEBUG_TEST_ENABLE	 0
#endif /* DEBUG_ENABLE */

#if LOCAL_DEBUG_PRINT_ENABLE
#warning LOCAL_DEBUG_PRINT_ENABLE
#define LOCAL_DEBUG_PRINT	  DEBUG_PRINT_NL
#define LOCAL_DEBUG_LOG_PRINT DEBUG_LOG_PRINT
#else /* LOCAL_DEBUG_PRINT_ENABLE */
#define LOCAL_DEBUG_PRINT(_f_, ...)
#define LOCAL_DEBUG_LOG_PRINT(_f_, ...)
#endif /* LOCAL_DEBUG_PRINT_ENABLE */

static bool Storage_EmmcHw_InitState;
static bool Storage_EmmcFs_InitState;
static Pl_SdEmmcInfo_t Storage_EmmcInfo;
static FATFS Storage_EmmcFsObj;

static bool Storage_RamHw_InitState;
static bool Storage_RamFs_InitState;
static u8 PL_STORAGE_IN_RAM_DATA RamStorageBuff[APP_RAM_STORAGE_CAPACITY];
static FATFS Storage_RamFsObj;

static u32 Storage_ReadWriteOps_LastTime;

Pl_SdEmmcInfo_t Storage_GetEmmcInfo(void) {
	return Storage_EmmcInfo;
};

void Storage_Init(void) {
	char discLabel[32];

	FsWrap_Init();

	bool eMmcInit = Pl_Emmc_Init(&Storage_EmmcInfo);
	Delay_WaitTime_MilliSec(DELAY_1_MILSEC * 100);

	u32 emmcState = Pl_Emmc_GetCardState();
	DEBUG_LOG_LVL_PRINT(LOG_LVL_INFO, "eMMC state is %d", emmcState);
	DEBUG_LOG_LVL_PRINT(LOG_LVL_INFO, "eMMC mfg ID '%d', name '%s', rev '%d', SN '%d'",
						Storage_EmmcInfo.MfgID, Storage_EmmcInfo.ProdName, Storage_EmmcInfo.ProdRev,
						Storage_EmmcInfo.ProdSN);
	DEBUG_LOG_LVL_PRINT(LOG_LVL_INFO,
						"eMMC CardType %d, Class %d, RelCardAdd %d, BlockNbr %d, BlockSize %d, "
						"LogBlockNbr %d, LogBlockSize %d",
						Storage_EmmcInfo.CardType, Storage_EmmcInfo.Class,
						Storage_EmmcInfo.RelCardAdd, Storage_EmmcInfo.BlockNbr,
						Storage_EmmcInfo.BlockSize, Storage_EmmcInfo.LogBlockNbr,
						Storage_EmmcInfo.LogBlockSize);

	Storage_EmmcHw_InitState = eMmcInit && Pl_Emmc_IsCardInTransfer();
	if (Storage_EmmcHw_InitState) {
		FsWrap_Mount_t emmcMount = {
			.pFsData	   = &Storage_EmmcFsObj,
			.pMntPointPath = STORAGE_EMMC_ROOT_PATH,
		};

		RET_STATE_t res = FsWrap_Mount(&emmcMount);
		DEBUG_LOG_LVL_PRINT(LOG_LVL_INFO, "eMMC mount on '%s' with res %s", emmcMount.pMntPointPath,
							RetState_GetStr(res));
		if (res == RET_STATE_SUCCESS) {
			Storage_EmmcFs_InitState = true;

			FsWrap_SetLabel(STORAGE_EMMC_ROOT_PATH "prss rom");
			FsWrap_GetLabel(emmcMount.pMntPointPath, discLabel, NULL);
			DEBUG_LOG_LVL_PRINT(LOG_LVL_INFO, "Storage eMMC FS inited on '%s' with label '%s'",
								emmcMount.pMntPointPath, discLabel);
		} else {
			DEBUG_LOG_LVL_PRINT(LOG_LVL_ERROR, "Storage eMMC FS init failed!");
		}
	} else {
		DEBUG_LOG_LVL_PRINT(LOG_LVL_ERROR, "eMMC HW init failed!");
	}

	FsWrap_Mount_t ramMount = {
		.pFsData	   = &Storage_RamFsObj,
		.pMntPointPath = STORAGE_RAM_ROOT_PATH,
	};

	Storage_RamHw_InitState = true;

	RET_STATE_t res = FsWrap_Mount(&ramMount);
	DEBUG_LOG_LVL_PRINT(LOG_LVL_INFO, "RAM mounted on '%s' with res %s", ramMount.pMntPointPath,
						RetState_GetStr(res));
	if (res == RET_STATE_SUCCESS) {
		Storage_RamFs_InitState = true;
		ASSERT_CHECK(Storage_RamFs_InitState);

		char tmpBuff[32];
		snprintf(tmpBuff, sizeof(tmpBuff), "%s%s", ramMount.pMntPointPath, "prss ram");
		FsWrap_SetLabel(tmpBuff);
		FsWrap_GetLabel(ramMount.pMntPointPath, discLabel, NULL);
		DEBUG_LOG_LVL_PRINT(LOG_LVL_INFO, "Storage RAM FS inited on '%s' with label '%s'",
							ramMount.pMntPointPath, discLabel);
	} else {
		DEBUG_LOG_LVL_PRINT(LOG_LVL_ERROR, "Storage RAM init failed!");
	}
}

bool Storage_EmmcHw_IsInit(void) {
	return Storage_EmmcHw_InitState;
};

bool Storage_EmmcFs_IsInit(void) {
	return Storage_EmmcFs_InitState;
};

bool Storage_Emmc_GetDeviceInfo(Pl_SdEmmcInfo_t* pSdEmmcInfo) {
	return Pl_Emmc_GetDeviceInfo(pSdEmmcInfo);
}

bool Storage_Emmc_Read(u8* pData, u32 blockIdx, u32 blockNum) {
	LOCAL_DEBUG_LOG_PRINT("R: from %d with len %d\r\n", offset, len);
	Storage_ReadWriteOps_LastTime = PL_GET_MS_CNT();
	return Pl_Emmc_Read(pData, blockIdx, blockNum);
}

bool Storage_Emmc_Write(const u8* pData, u32 blockIdx, u32 blockNum) {
	LOCAL_DEBUG_LOG_PRINT("W: to %d with len %d\r\n", offset, len);
	Storage_ReadWriteOps_LastTime = PL_GET_MS_CNT();
	return Pl_Emmc_Write(pData, blockIdx, blockNum);
}

u32 Storage_Emmc_GetCardState(void) {
	return Pl_Emmc_GetCardState();
}

u32 Storage_Emmc_IsCardInTransfer(void) {
	return Pl_Emmc_IsCardInTransfer();
}

bool Storage_RamHw_IsInit(void) {
	return Storage_RamHw_InitState;
};

bool Storage_RamFs_IsInit(void) {
	return Storage_RamFs_InitState;
};

u8* Storage_Ram_GetStoragePtr(void) {
	return RamStorageBuff;
}

u32 Storage_Ram_GetStorageCap(void) {
	return sizeof(RamStorageBuff);
}

void Storage_Ram_Read(u8* pDst, u32 offset, u32 len) {
	Storage_ReadWriteOps_LastTime = PL_GET_MS_CNT();
	LOCAL_DEBUG_LOG_PRINT("R: from %d with len %d\r\n", offset, len);

	if (offset + len > Storage_Ram_GetStorageCap()) {
		PANIC();
		return;
	}

	memcpy((void*)pDst, (void*)&RamStorageBuff[offset], len);
}

void Storage_Ram_Write(const u8* pSrc, u32 offset, u32 len) {
	Storage_ReadWriteOps_LastTime = PL_GET_MS_CNT();
	LOCAL_DEBUG_LOG_PRINT("W: to %d with len %d\r\n", offset, len);

	if (offset + len > Storage_Ram_GetStorageCap()) {
		PANIC();
		return;
	}

	memcpy((void*)&RamStorageBuff[offset], (void*)pSrc, len);
}

u32 Storage_GetReadWriteOps_LastTime(void) {
	return Storage_ReadWriteOps_LastTime;
}
