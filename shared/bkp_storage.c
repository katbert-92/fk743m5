#include "bkp_storage.h"
#include "mathlib_common.h"
#include "platform.h"

#if BKP_STORAGE_RAM_EMULATION

volatile u32 PL_BKP_STORAGE_DATA BkpStorageRam[PL_BKP_STORAGE_MAX_LEN];

static u32 BkpStorage_CalcStorageHash(void) {
	return jenkins_hash((unsigned char*)BkpStorageRam,
						sizeof(BkpStorageRam) - sizeof(BkpStorageRam[0]));
}

static void BkpStorage_SaveStorageHash(u32 hash) {
	BkpStorageRam[PL_BKP_STORAGE_MAX_LEN - 1] = hash;
}

u32 BkpStorage_GetValue(BKP_STORAGE_KEY_t key) {
	ASSERT_CHECK(key < NUM_ELEMENTS(BkpStorageRam));

	if (key >= NUM_ELEMENTS(BkpStorageRam))
		return 0;

	return BkpStorageRam[key];
}

void BkpStorage_SetValue(BKP_STORAGE_KEY_t key, u32 data) {
	ASSERT_CHECK(key < NUM_ELEMENTS(BkpStorageRam));

	if (key >= NUM_ELEMENTS(BkpStorageRam))
		return;

	BkpStorageRam[key] = data;
	u32 hashCalc	   = BkpStorage_CalcStorageHash();
	BkpStorage_SaveStorageHash(hashCalc);
}

#else /* BKP_STORAGE_RAM_EMULATION */

u32 BkpStorage_GetValue(BKP_STORAGE_KEY_t key) {
	ASSERT_CHECK(key < NUM_ELEMENTS(BkpStorageRam));

	if (key >= NUM_ELEMENTS(BkpStorageRam))
		return 0;

	return 0;
}

void BkpStorage_SetValue(BKP_STORAGE_KEY_t key, u32 data) {
	ASSERT_CHECK(key < NUM_ELEMENTS(BkpStorageRam));

	if (key >= NUM_ELEMENTS(BkpStorageRam))
		return;
}

#endif /* BKP_STORAGE_RAM_EMULATION */

void BkpStorage_Init(void) {
	u32 hashCalc  = BkpStorage_CalcStorageHash();
	u32 hashSaved = BkpStorage_GetValue(PL_BKP_STORAGE_MAX_LEN - 1);

	if (hashCalc != hashSaved) {
		memset((void*)BkpStorageRam, 0, sizeof(BkpStorageRam));
		hashCalc = BkpStorage_CalcStorageHash();
		BkpStorage_SaveStorageHash(hashCalc);
	}
}
