#ifndef __STORAGE_H
#define __STORAGE_H

#include "main.h"
#include "platform.h"

typedef enum {
	STORAGE_DRIVE_EMMC = 0,
	STORAGE_DRIVE_RAM,

	STORAGE_DRIVE_ENUM_SIZE
} STORAGE_DRIVE_t;

/**
 * Should be aligned by 512 bytes and not less then
 * 128 sectors + N_SEC_TRACK sectors 
 * + 2 for 50Kb view on PC
 */
#define APP_RAM_STORAGE_CAPACITY ((128 + 63 + 2) * (DATA_1_KBYTE / 2))	//400 for bigger at last

#define STORAGE_EMMC_ROOT_PATH "0:"
#define STORAGE_RAM_ROOT_PATH  "1:"

Pl_SdEmmcInfo_t Storage_GetEmmcInfo(void);

void Storage_Init(void);

bool Storage_EmmcHw_IsInit(void);
bool Storage_EmmcFs_IsInit(void);

bool Storage_Emmc_GetDeviceInfo(Pl_SdEmmcInfo_t* pSdEmmcInfo);
bool Storage_Emmc_Read(u8* pData, u32 blockIdx, u32 blockNum);
bool Storage_Emmc_Write(const u8* pData, u32 blockIdx, u32 blockNum);
u32 Storage_Emmc_GetCardState(void);
u32 Storage_Emmc_IsCardInTransfer(void);

bool Storage_RamHw_IsInit(void);
bool Storage_RamFs_IsInit(void);

u8* Storage_Ram_GetStoragePtr(void);
u32 Storage_Ram_GetStorageCap(void);
void Storage_Ram_Read(u8* pDst, u32 offset, u32 len);
void Storage_Ram_Write(const u8* pSrc, u32 offset, u32 len);

u32 Storage_GetReadWriteOps_LastTime(void);

#endif /* __STORAGE_H */
