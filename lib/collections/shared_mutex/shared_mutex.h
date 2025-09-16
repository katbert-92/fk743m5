#ifndef SHARED_MUTEX_H
#define SHARED_MUTEX_H

#include "main.h"

#define SHARED_MUTEX_AWAIT_UNLOCK_MS 2

typedef enum {
	SHARED_MUTEX_LOCK_NO	= 0x00,
	SHARED_MUTEX_LOCK_WR	= 0x01,
	SHARED_MUTEX_LOCK_RE_WR = 0x02
} SHARED_MUTEX_LOCK_TYPE_t;

typedef u32 (*SharedMutex_GetMs_Fptr_t)(void);
typedef void (*SharedMutex_WaitMs_Fptr_t)(const u32 waitMs);

typedef struct {
	SHARED_MUTEX_LOCK_TYPE_t LockType;
	SharedMutex_GetMs_Fptr_t GetMs;
	SharedMutex_WaitMs_Fptr_t WaitMs;
	u32 ConcurentReaders;
	bool RejectNewReaders;
	u32 LockKey;
} SharedMutex_t;

void SharedMutex_Init(SharedMutex_t* pAccess, SharedMutex_GetMs_Fptr_t fpGetMs,
					  SharedMutex_WaitMs_Fptr_t fpWaitMs);
RET_STATE_t SharedMutex_WriteLock(SharedMutex_t* pAccess, u32 waitMs);
RET_STATE_t SharedMutex_WriteUnlock(SharedMutex_t* pAccess);
u32 SharedMutex_GetConcurentReaders(SharedMutex_t* pAccess);
RET_STATE_t SharedMutex_ReadWriteLock(SharedMutex_t* pAccess, u32 waitMs, u32* pLockKey);
RET_STATE_t SharedMutex_ReadWriteUnlock(SharedMutex_t* pAccess);
bool SharedMutex_IsLocked(SharedMutex_t* pAccess, u32 extLockKey);

#endif /* SHARED_MUTEX_H */
