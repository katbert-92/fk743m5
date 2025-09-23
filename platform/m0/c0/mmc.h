#ifndef __MMC_H
#define __MMC_H

#include "main.h"
#include "platform.h"
#include "platform_inc_m0.h"

#define EMMC_IRQ SDMMC1_IRQn

bool MMC_Emmc_Init(u32* pMfgId, char* pProdName, u32* pProdRev, u32* pProdSn);
bool MMC_Emmc_ReadBlocks(u8* pData, u32 blockIdx, u32 blockNum, u32 tmo);
bool MMC_Emmc_WriteBlocks(const u8* pData, u32 blockIdx, u32 blockNum, u32 tmo);
bool MMC_Emmc_ReadBlocksDMA(u8* pData, u32 blockIdx, u32 blockNum);
bool MMC_Emmc_WriteBlocksDMA(const u8* pData, u32 blockIdx, u32 blockNum);
bool MMC_Emmc_Erase(u32 startAddr, u32 endAddr);
u32 MMC_Emmc_GetCardState(void);
bool MMC_Emmc_IsCardInTransfer(void);
bool MMC_Emmc_GetDeviceInfo(HAL_MMC_CardInfoTypeDef* pDeviceInfo);

#endif /* __MMC_H */
