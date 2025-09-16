#include "shared_mutex.h"

#if DEBUG_ENABLE
#include "debug.h"

#define LOCAL_DEBUG_PRINT_ENABLE 0
#endif /* DEBUG_ENABLE */

#if LOCAL_DEBUG_PRINT_ENABLE
#warning LOCAL_DEBUG_PRINT_ENABLE
#define LOCAL_DEBUG_PRINT		DEBUG_LOG_PRINT
#define LOCAL_DEBUG_COLOR_PRINT DEBUG_LOG_COLOR_PRINT
#else /* LOCAL_DEBUG_PRINT_ENABLE */
#define LOCAL_DEBUG_PRINT(_f_, ...)
#define LOCAL_DEBUG_COLOR_PRINT(c, _f_, ...)
#endif /* LOCAL_DEBUG_PRINT_ENABLE */

#ifndef SHARED_MUTEX_CUSTOM_RAND
#include "rand.h"

#ifndef LL_GET_RAND
#define LL_GET_RAND() Rand_GetNum()
#endif /* LL_GET_RAND */
#endif /* LINKED_LIST_CUSTOM_RAND */

void SharedMutex_Init(SharedMutex_t* pAccess, SharedMutex_GetMs_Fptr_t fpGetMs,
					  SharedMutex_WaitMs_Fptr_t fpWaitMs) {
	*pAccess = (SharedMutex_t){
		.LockType		  = SHARED_MUTEX_LOCK_NO,
		.GetMs			  = fpGetMs,
		.WaitMs			  = fpWaitMs,
		.ConcurentReaders = 0,
		.RejectNewReaders = false,
		.LockKey		  = 0,
	};
}

/**
 * @brief Tries to lock SharedMutex on WR
 * 
 * @param[in] pAccess pointer on SharedMutex object
 * @param[in] waitMs await delay for locking try timeout
 * @retval RET_STATE_ERR_PARAM bad input parameter 
 * @retval RET_STATE_ERR_BUSY SharedMutex locked on RE_WR or new readers rejected
 * @retval RET_STATE_SUCCESS SharedMutex locked on WR successfully
 */
RET_STATE_t SharedMutex_WriteLock(SharedMutex_t* pAccess, u32 waitMs) {
	if (!pAccess) {
		PANIC();
		return RET_STATE_ERR_PARAM;
	}

	if (!pAccess->GetMs && !pAccess->WaitMs)
		return RET_STATE_SUCCESS;

	if (pAccess->RejectNewReaders) {
		LOCAL_DEBUG_COLOR_PRINT(ESC_COLOR_YELLOW, "New readers rejected! ");
		return RET_STATE_ERR_BUSY;
	}

	RET_STATE_t lockResult = RET_STATE_UNDEF;
	switch (pAccess->LockType) {
		case SHARED_MUTEX_LOCK_RE_WR: {
			u32 timeoutEnd = pAccess->GetMs() + waitMs;
			while (pAccess->LockType == SHARED_MUTEX_LOCK_RE_WR) {
				pAccess->WaitMs(SHARED_MUTEX_AWAIT_UNLOCK_MS);
				if (pAccess->GetMs() > timeoutEnd) {
					lockResult = RET_STATE_ERR_BUSY;
					LOCAL_DEBUG_COLOR_PRINT(ESC_COLOR_YELLOW, "rwLock off await timeout");
					break;
				}
			}

			if (lockResult == RET_STATE_ERR_BUSY)
				break;
			// else - fallthrough
		}

		case SHARED_MUTEX_LOCK_NO:
			SYS_CRITICAL_ON();
			pAccess->LockType		  = SHARED_MUTEX_LOCK_WR;
			pAccess->ConcurentReaders = 1;
			lockResult				  = RET_STATE_SUCCESS;
			SYS_CRITICAL_OFF();
			break;

		case SHARED_MUTEX_LOCK_WR:
			SYS_CRITICAL_ON();
			pAccess->ConcurentReaders++;
			lockResult = RET_STATE_SUCCESS;
			SYS_CRITICAL_OFF();
			break;

		default:
			lockResult = RET_STATE_ERR_PARAM;
	}

	LOCAL_DEBUG_COLOR_PRINT(ESC_COLOR_GREEN, "wLock status: %s, concurent readers: %d",
							RetState_GetStr(lockResult), pAccess->ConcurentReaders);
	return lockResult;
}

/**
 * @brief Unlock SharedMutex after WR lock
 * 
 * @param[in] pAccess pointer on SharedMutex object
 * @retval RET_STATE_ERR_PARAM bad input parameter 
 * @retval RET_STATE_ERR_BUSY SharedMutex locked on RE_WR
 * @retval RET_STATE_SUCCESS SharedMutex unlocked from WR successfuly
 */
RET_STATE_t SharedMutex_WriteUnlock(SharedMutex_t* pAccess) {
	if (!pAccess) {
		PANIC();
		return RET_STATE_ERR_PARAM;
	}

	SYS_CRITICAL_ON();

	RET_STATE_t unlockResult = RET_STATE_UNDEF;
	switch (pAccess->LockType) {
		case SHARED_MUTEX_LOCK_NO:
			unlockResult = RET_STATE_SUCCESS;
			break;

		case SHARED_MUTEX_LOCK_WR:
			if (pAccess->ConcurentReaders == 0) {
				//Just try to be sure this case is impossible
				LOCAL_DEBUG_COLOR_PRINT(ESC_COLOR_RED, "Concurent readers is zero!");
				PANIC();
			}
			if (--pAccess->ConcurentReaders == 0)
				pAccess->LockType = SHARED_MUTEX_LOCK_NO;

			unlockResult = RET_STATE_SUCCESS;
			break;

		case SHARED_MUTEX_LOCK_RE_WR:
			unlockResult = RET_STATE_ERR_BUSY;
			break;

		default:
			unlockResult = RET_STATE_ERR_PARAM;
	}

	SYS_CRITICAL_OFF();
	LOCAL_DEBUG_COLOR_PRINT(ESC_COLOR_GREEN, "wUnlock status: %s, concurent readers: %d",
							RetState_GetStr(unlockResult), pAccess->ConcurentReaders);
	return unlockResult;
}

/**
 * @brief Get concurent readers counter of SharedMutex object
 * 
 * @param[in] pAccess pointer on SharedMutex object
 * @retval concurent readers counter
 */
u32 SharedMutex_GetConcurentReaders(SharedMutex_t* pAccess) {
	if (!pAccess) {
		PANIC();
		return 0;
	}

	return pAccess->ConcurentReaders;
}

/**
 * @brief Tries to lock SharedMutex on RE_WR
 * 
 * @param[in] pAccess pointer on SharedMutex object
 * @param[in] waitMs await delay for locking try timeout
 * @param[out] pLockKey (optional) pointer to lock key variable for access to private ops
 * @retval RET_STATE_ERR_PARAM bad input parameter 
 * @retval RET_STATE_ERR_BUSY SharedMutex locked on RE_WR or old reades still subscribed
 * on SharedMutex within waitMs tmo
 * @retval RET_STATE_SUCCESS SharedMutex locked on WR successfuly
 * 
 * It has internal call of SharedMutex_WriteUnlock, so the result of
 * this function could be returned
 */
RET_STATE_t SharedMutex_ReadWriteLock(SharedMutex_t* pAccess, u32 waitMs, u32* pLockKey) {
	if (!pAccess) {
		PANIC();
		return RET_STATE_ERR_PARAM;
	}

	if (!pAccess->GetMs && !pAccess->WaitMs)
		return RET_STATE_SUCCESS;

	//TODO RETURN if the same lockID (PANIC)

	/**
	 * If SharedMutex already locked on RE_WR, wait for waitMs timeout
	 */
	if (pAccess->LockType == SHARED_MUTEX_LOCK_RE_WR) {
		u32 timeoutEnd = pAccess->GetMs() + waitMs;
		while (pAccess->LockType == SHARED_MUTEX_LOCK_RE_WR) {
			pAccess->WaitMs(SHARED_MUTEX_AWAIT_UNLOCK_MS);
			if (pAccess->GetMs() > timeoutEnd) {
				LOCAL_DEBUG_COLOR_PRINT(ESC_COLOR_YELLOW, "rwLock operation rejected: is locked");
				return RET_STATE_ERR_BUSY;
			}
		}
	}

	/**
	 * Firstly try to wLock for prevent any changes in data
	 */
	RET_STATE_t wrLockRes = SharedMutex_WriteLock(pAccess, waitMs);
	RETURN_IF_UNSUCCESS(wrLockRes);

	/**
	 * Here we go in case of successful wLock
	 * (LockType == SHARED_MUTEX_LOCK_WR)
	 */

	pAccess->RejectNewReaders = true;

	u32 timeoutEnd = pAccess->GetMs() + waitMs;
	while (pAccess->ConcurentReaders != 1) {
		pAccess->WaitMs(SHARED_MUTEX_AWAIT_UNLOCK_MS);
		if (pAccess->GetMs() > timeoutEnd) {
			SharedMutex_WriteUnlock(pAccess);
			pAccess->RejectNewReaders = false;

			LOCAL_DEBUG_COLOR_PRINT(ESC_COLOR_YELLOW, "Await old readers timeout");
			return RET_STATE_ERR_BUSY;
		}
	}

	SYS_CRITICAL_ON();
	pAccess->RejectNewReaders = false;
	pAccess->LockType		  = SHARED_MUTEX_LOCK_RE_WR;
	while (!pAccess->LockKey)
		pAccess->LockKey = LL_GET_RAND();

	if (pLockKey)
		*pLockKey = pAccess->LockKey;

	SYS_CRITICAL_OFF();
	LOCAL_DEBUG_COLOR_PRINT(ESC_COLOR_GREEN, "rwLock successful, lockID: %8x", pAccess->LockKey);
	return RET_STATE_SUCCESS;
}

/**
 * @brief Unlock SharedMutex after RE_WR lock
 * 
 * @param[in] pAccess pointer on SharedMutex object
 * @retval RET_STATE_ERR_PARAM bad input parameter 
 * @retval RET_STATE_ERR_BUSY SharedMutex locked on WR by other thread
 * @retval RET_STATE_SUCCESS SharedMutex unlocked from RE_WR successfuly
 */
RET_STATE_t SharedMutex_ReadWriteUnlock(SharedMutex_t* pAccess) {
	if (!pAccess) {
		PANIC();
		return RET_STATE_ERR_PARAM;
	}

	SYS_CRITICAL_ON();

	RET_STATE_t unlockResult = RET_STATE_UNDEF;
	switch (pAccess->LockType) {
		case SHARED_MUTEX_LOCK_NO:
			unlockResult = RET_STATE_SUCCESS;
			break;

		case SHARED_MUTEX_LOCK_WR:
			unlockResult = RET_STATE_ERR_BUSY;
			break;

		case SHARED_MUTEX_LOCK_RE_WR:
			if (pAccess->ConcurentReaders != 1) {
				//Just try to be sure this case is impossible
				LOCAL_DEBUG_COLOR_PRINT(ESC_COLOR_RED, "ConcurentReaders is not zero!");
				PANIC();
			}
			pAccess->ConcurentReaders--;
			pAccess->LockType = SHARED_MUTEX_LOCK_NO;
			pAccess->LockKey  = 0;

			unlockResult = RET_STATE_SUCCESS;
			break;
	}

	SYS_CRITICAL_OFF();
	LOCAL_DEBUG_COLOR_PRINT(ESC_COLOR_GREEN, "rwUnlock status: %s, concurent readers: %d",
							RetState_GetStr(unlockResult), pAccess->ConcurentReaders);
	return unlockResult;
}

/**
 * @brief Internal function for any SharedMutex lock type check. If SharedMutex
 * is locked - it returns true, but if the extLockKey is the same with LockKey - 
 * it starts to be unlocked for ops
 * 
 * @param[in] pAccess pointer on SharedMutex object
 * @param[in] extLockKey lock key for private access
 * @retval is locked/unlocked
 */
bool SharedMutex_IsLocked(SharedMutex_t* pAccess, u32 extLockKey) {
	if (!pAccess) {
		PANIC();
		return false;
	}

	SYS_CRITICAL_ON();

	/**
	 * Check if the object mutex is free; in case of
	 * SHARED_MUTEX_LOCK_RE_WR lock, LockKey needs to be
	 * checked for equality with extLockKey
	 */
	bool isLocked = false;
	if (pAccess->LockType != SHARED_MUTEX_LOCK_NO) {
		isLocked = true;

		if (pAccess->LockType == SHARED_MUTEX_LOCK_RE_WR && pAccess->LockKey == extLockKey) {
			isLocked = false;
			LOCAL_DEBUG_COLOR_PRINT(
				ESC_COLOR_GREEN, "External lock key the same with internal: %8x", pAccess->LockKey);
		}
	}

	SYS_CRITICAL_OFF();
	LOCAL_DEBUG_COLOR_PRINT(ESC_COLOR_GREEN, "Is locked: %d, lock type: %d", isLocked,
							pAccess->LockType);
	return isLocked;
}
