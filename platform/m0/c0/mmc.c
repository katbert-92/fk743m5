#include "mmc.h"

static MMC_HandleTypeDef EMMC_Handle;

bool MMC_Emmc_Init(u32* pMfgId, char* pProdName, u32* pProdRev, u32* pProdSn) {
	if (!Pl_IsInit.Sys || !Pl_IsInit.DelayMs) {
		PANIC();
		return false;
	}

	// EMMC_Handle.Instance = SDMMC1;
	// HAL_MMC_DeInit(&EMMC_Handle);
	LL_AHB3_GRP1_EnableClock(LL_AHB3_GRP1_PERIPH_SDMMC1);

	EMMC_Handle.Instance				 = SDMMC1;
	EMMC_Handle.Init.ClockDiv			 = 8;  // 200Mhz / (2 * ClockDiv)
	EMMC_Handle.Init.ClockPowerSave		 = SDMMC_CLOCK_POWER_SAVE_DISABLE;
	EMMC_Handle.Init.ClockEdge			 = SDMMC_CLOCK_EDGE_RISING;
	EMMC_Handle.Init.HardwareFlowControl = SDMMC_HARDWARE_FLOW_CONTROL_DISABLE;
	EMMC_Handle.Init.BusWide			 = SDMMC_BUS_WIDE_1B;

	if (HAL_MMC_Init(&EMMC_Handle) != HAL_OK)
		return false;

	// if (HAL_MMC_ConfigWideBusOperation(&EMMC_Handle, SDMMC_BUS_WIDE_4B) != HAL_OK) {
	// 	return false;
	// }

	// if (HAL_MMC_ConfigSpeedBusOperation(&EMMC_Handle, SDMMC_SPEED_MODE_DEFAULT) != HAL_OK) {
	// 	return false;
	// }

	HAL_MMC_CardCIDTypeDef cid = {0};
	HAL_MMC_GetCardCID(&EMMC_Handle, &cid);
	*pMfgId		 = cid.ManufacturerID;
	pProdName[0] = (char)((cid.ProdName1 >> 24) & 0xFF);
	pProdName[1] = (char)((cid.ProdName1 >> 16) & 0xFF);
	pProdName[2] = (char)((cid.ProdName1 >> 8) & 0xFF);
	pProdName[3] = (char)(cid.ProdName1 & 0xFF);
	pProdName[4] = (char)cid.ProdName2;
	pProdName[5] = '\0';
	*pProdRev	 = cid.ProdRev;
	*pProdSn	 = cid.ProdSN;

	HAL_MMC_CardCSDTypeDef csd = {0};
	HAL_MMC_GetCardCSD(&EMMC_Handle, &csd);

	return true;
}

bool MMC_Emmc_ReadBlocks(u8* pData, u32 blockIdx, u32 blockNum, u32 tmo) {
	u32 timeout = tmo * blockNum;
	if (HAL_MMC_ReadBlocks(&EMMC_Handle, (u8*)pData, blockIdx, blockNum, timeout) != HAL_OK) {
		return false;
	}

	return true;
}

bool MMC_Emmc_WriteBlocks(const u8* pData, u32 blockIdx, u32 blockNum, u32 tmo) {
	u32 timeout = tmo * blockNum;
	if (HAL_MMC_WriteBlocks(&EMMC_Handle, (u8*)pData, blockIdx, blockNum, timeout) != HAL_OK) {
		return false;
	}

	return true;
}

bool MMC_Emmc_ReadBlocksDMA(u8* pData, u32 blockIdx, u32 blockNum) {
	if (HAL_MMC_ReadBlocks_DMA(&EMMC_Handle, (u8*)pData, blockIdx, blockNum) != HAL_OK) {
		return false;
	}

	return true;
}

bool MMC_Emmc_WriteBlocksDMA(const u8* pData, u32 blockIdx, u32 blockNum) {
	if (HAL_MMC_WriteBlocks_DMA(&EMMC_Handle, (u8*)pData, blockIdx, blockNum) != HAL_OK) {
		return false;
	}

	return true;
}

bool MMC_Emmc_Erase(u32 startAddr, u32 endAddr) {
	if (HAL_MMC_Erase(&EMMC_Handle, startAddr, endAddr) != HAL_OK) {
		return false;
	}

	return true;
}

u32 MMC_Emmc_GetCardState(void) {
	return HAL_MMC_GetCardState(&EMMC_Handle);
}

bool MMC_Emmc_IsCardInTransfer(void) {
	return HAL_MMC_GetCardState(&EMMC_Handle) == HAL_MMC_CARD_TRANSFER ? true : false;
}

bool MMC_Emmc_GetDeviceInfo(HAL_MMC_CardInfoTypeDef* pDeviceInfo) {
	if (HAL_MMC_GetCardInfo(&EMMC_Handle, pDeviceInfo) != HAL_OK) {
		return false;
	}

	return true;
}

void SDMMC1_IRQHandler(void) {
	HAL_MMC_IRQHandler(&EMMC_Handle);
}
