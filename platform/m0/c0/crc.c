#include "crc.h"

bool CRC_Init(void) {
	if (!Pl_IsInit.Sys) {
		PANIC();
		return false;
	}

	LL_AHB4_GRP1_EnableClock(LL_AHB4_GRP1_PERIPH_CRC);
	LL_CRC_ResetCRCCalculationUnit(CRC);
	LL_CRC_SetInputDataReverseMode(CRC, LL_CRC_INDATA_REVERSE_BYTE);
	LL_CRC_SetOutputDataReverseMode(CRC, LL_CRC_OUTDATA_REVERSE_BIT);

	return true;
}

void CRC_Reset(void) {
	LL_CRC_ResetCRCCalculationUnit(CRC);
}

u32 CRC_CheckBuff(const u8* pBuff, u32 buffSize) {
	LL_CRC_ResetCRCCalculationUnit(CRC);

	u32 data = 0;
	u32 idx	 = 0;

	for (idx = 0; idx < (buffSize / 4); idx++) {
		data = (u32)((pBuff[4 * idx + 0] << 24) | (pBuff[4 * idx + 1] << 16) |
					 (pBuff[4 * idx + 2] << 8) | (pBuff[4 * idx + 3]));

		LL_CRC_FeedData32(CRC, data);
	}

	if (buffSize % 4 != 0) {
		switch (buffSize % 4) {
			case 1:
				LL_CRC_FeedData8(CRC, pBuff[4 * idx]);
				break;
			case 2:
				LL_CRC_FeedData16(CRC, (u16)((pBuff[4 * idx + 0] << 8) | (pBuff[4 * idx + 1])));
				break;
			case 3:
				LL_CRC_FeedData16(CRC, (u16)((pBuff[4 * idx + 0] << 8) | (pBuff[4 * idx + 1])));
				LL_CRC_FeedData8(CRC, pBuff[4 * idx + 2]);
				break;
		}
	}

	return LL_CRC_ReadData32(CRC);
}
