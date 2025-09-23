#ifndef __BKP_STORAGE_H
#define __BKP_STORAGE_H

#include "main.h"

#define BKP_STORAGE_RAM_EMULATION 1

typedef enum {
	BKP_KEY_DEADBEEF = 0,
	BKP_KEY_RTC_STATE,
	BKP_KEY_CALEND_MARKER,
	BKP_KEY_SYS_FAULT_EXEPTION_ADDR,

	BKP_KEY_ENUM_SIZE
} BKP_STORAGE_KEY_t;

u32 BkpStorage_GetValue(BKP_STORAGE_KEY_t key);
void BkpStorage_SetValue(BKP_STORAGE_KEY_t key, u32 data);
void BkpStorage_Init(void);

#endif /* __BKP_STORAGE_H */
