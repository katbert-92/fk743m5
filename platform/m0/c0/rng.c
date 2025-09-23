#include "rng.h"

#define RNG_DATA_READY_TMO_MS 100

bool RNG_Init(void) {
	if (!Pl_IsInit.Sys || !Pl_IsInit.DelayMs) {
		PANIC();
		return false;
	}

	LL_AHB2_GRP1_EnableClock(LL_AHB2_GRP1_PERIPH_RNG);

	LL_RNG_InitTypeDef RNG_InitStruct;
	LL_RNG_StructInit(&RNG_InitStruct);

	RNG_InitStruct.ClockErrorDetection = LL_RNG_CED_DISABLE;
	LL_RNG_Init(RNG, &RNG_InitStruct);

	return true;
}

bool RNG_GenerateBuff(u32* pBuff, u32 maxNum) {
	LL_RNG_Enable(RNG);

	for (u32 idx = 0; idx < maxNum; idx++) {
		u32 endTime = Pl_DelayMs_GetMsCnt() + RNG_DATA_READY_TMO_MS;

		while (!LL_RNG_IsActiveFlag_DRDY(RNG)) {
			if (Pl_DelayMs_GetMsCnt() > endTime) {
				LL_RNG_Disable(RNG);
				return false;  // timeout
			}
		}

		// Check for seed error
		if (LL_RNG_IsActiveFlag_SECS(RNG)) {
			LL_RNG_Disable(RNG);
			return false;
		}

		pBuff[idx] = LL_RNG_ReadRandData32(RNG);

		if (pBuff[idx] == 0) {
			LL_RNG_Disable(RNG);
			return false;
		}
	}

	LL_RNG_Disable(RNG);
	return true;
}
