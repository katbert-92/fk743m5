#include "linked_list.h"
#include "stringlib.h"

#ifndef LINKED_LIST_CUSTOM_LIBC

#ifndef LL_MEMSET
#define LL_MEMSET(d, c, n) memset((d), (c), (n))
#endif /* LS_MEMSET */

#ifndef LL_MEMCPY
#define LL_MEMCPY(d, s, n) memcpy((d), (s), (n))
#endif /* LS_MEMSET */
#endif /* LINKED_LIST_CUSTOM_LIBC */

#ifndef LINKED_LIST_CUSTOM_RAND

#ifndef LL_GET_RAND
#define LL_GET_RAND() rand()
#endif /* LL_GET_RAND */
#endif /* LINKED_LIST_CUSTOM_RAND */

#ifndef LINKED_LIST_CUSTOM_ALLOCS
#include "mem_wrapper.h"

#ifndef LL_MALLOC
#define LL_MALLOC(s, pf, l, t) MemWrap_Malloc((s), (pf), (l), (t))
#endif /* LL_MALLOC */

#ifndef LL_FREE
#define LL_FREE(pa) MemWrap_Free((pa))
#endif /* LL_FREE */

#ifndef LL_GET_FREE_HEAP
#define LL_GET_FREE_HEAP(pa) MemWrap_GetFreeHeapSize()
#endif /* LL_GET_FREE_HEAP */
#endif /* LINKED_LIST_CUSTOM_ALLOCS */

#if DEBUG_ENABLE
#include "debug.h"

#define LOCAL_DEBUG_PRINT_ENABLE 0	//default 0
#define LOCAL_DEBUG_TEST_ENABLE	 0
#endif /* DEBUG_ENABLE */

#if LOCAL_DEBUG_PRINT_ENABLE
#warning LOCAL_DEBUG_PRINT_ENABLE
#define LOCAL_DEBUG_PRINT DEBUG_LOG_PRINT
#else /* LOCAL_DEBUG_PRINT_ENABLE */
#define LOCAL_DEBUG_PRINT(_f_, ...)
#endif /* LOCAL_DEBUG_PRINT_ENABLE */

#if LOCAL_DEBUG_TEST_ENABLE
#warning LOCAL_DEBUG_TEST_ENABLE
#endif /* LOCAL_DEBUG_TEST_ENABLE */

// [####] - is a data in memory
// LinkedList nodes chain structure:
// [0]    [1]    [2]       [n]
// [Rear0][Node1][Node2]...[FrontN][NULL]
// -------------------------------------------------------------------------
// [pRear][Metadata][Metrics][Access]..............| LinkedList instance
//  \      \         \                             |
//   \      \         [NodesNum][BytesNum].........| LinkedList metrics
//    \      \                                     |
//     \      [Data addr][Size]....................| LinkedList metadata
//      \                                          |
//       [pNext][Data][####].......................| Node 0 (rear)
//        \      \     ^                           |
//         \      \    |                           |
//          \      [Data addr][Size]...............| LinkedList data information
//           \                                     |
//            [pNext][Data][####]..................| Node 1
//             \                                   |
//              [pNext][Data][####]................| Node 2 (front)
//               \                                 |
//                NULL.............................| Pointer to the void

/**
 * @brief Tries to lock LinkedList on WR
 * 
 * @param[in] pcHandle handle for LL object
 * @param[in] waitMs await delay for locking try timeout
 * @retval RET_STATE_ERR_PARAM bad input parameter 
 * @retval RET_STATE_ERR_EMPTY LL isn't inited
 * @retval RET_STATE_ERR_BUSY LL locked on RE_WR or new readers rejected
 * @retval RET_STATE_SUCCESS LL locked on WR successfuly
 */
RET_STATE_t LinkedList_WriteLock(LinkedList_Handle_t* const pcHandle, u32 waitMs) {
	if (!pcHandle) {
		PANIC();
		return RET_STATE_ERR_PARAM;
	}

	if (!(*pcHandle))
		return RET_STATE_ERR_EMPTY;

	SharedMutex_t* pAccess = &((*pcHandle)->Access);
	RET_STATE_t lockResult = SharedMutex_WriteLock(pAccess, waitMs);

	LOCAL_DEBUG_PRINT("LL wLock status: %s, concurReaders: %d", RetState_GetStr(lockResult),
					  pAccess->ConcurentReaders);
	return lockResult;
}

/**
 * @brief Unlock LinkedList after WR lock
 * 
 * @param[in] pcHandle handle for LL object
 * @retval RET_STATE_ERR_PARAM bad input parameter 
 * @retval RET_STATE_ERR_EMPTY LL isn't inited
 * @retval RET_STATE_ERR_BUSY LL locked on RE_WR
 * @retval RET_STATE_SUCCESS LL unlocked from WR successfuly
 */
RET_STATE_t LinkedList_WriteUnlock(LinkedList_Handle_t* const pcHandle) {
	if (!pcHandle) {
		PANIC();
		return RET_STATE_ERR_PARAM;
	}

	if (!(*pcHandle))
		return RET_STATE_ERR_EMPTY;

	SharedMutex_t* pAccess	 = &((*pcHandle)->Access);
	RET_STATE_t unlockResult = SharedMutex_WriteUnlock(pAccess);

	LOCAL_DEBUG_PRINT("LL wUnlock status: %s, concurReaders: %d", RetState_GetStr(unlockResult),
					  pAccess->ConcurentReaders);
	return unlockResult;
}

/**
 * @brief Get concurent readers counter of LinkedList object
 * 
 * @param[in] pcHandle handle for LL object
 * @retval concurent readers counter
 */
u32 LinkedList_GetConcurentReaders(LinkedList_Handle_t* const pcHandle) {
	if (!pcHandle) {
		PANIC();
		return 0;
	}

	if (!(*pcHandle))
		return 0;

	SharedMutex_t* pAccess = &((*pcHandle)->Access);
	return SharedMutex_GetConcurentReaders(pAccess);
}

/**
 * @brief Tries to lock LinkedList on RE_WR
 * 
 * @param[in] pcHandle handle for LL object
 * @param[in] waitMs await delay for locking try timeout
 * @param[out] pLockKey (optional) pointer to lock key variable for access to LL private ops
 * @retval RET_STATE_ERR_PARAM bad input parameter 
 * @retval RET_STATE_ERR_EMPTY LL isn't inited
 * @retval RET_STATE_ERR_BUSY LL locked on RE_WR or old reades still subscribed
 * on LL within waitMs tmo
 * @retval RET_STATE_SUCCESS LL locked on WR successfuly
 * @retval Internal call of LinkedList_WriteUnlock, so it possible
 * the result of this function will be returned
 */
RET_STATE_t LinkedList_ReadWriteLock(LinkedList_Handle_t* const pcHandle, u32 waitMs,
									 u32* pLockKey) {
	if (!pcHandle) {
		PANIC();
		return RET_STATE_ERR_PARAM;
	}

	if (!(*pcHandle))
		return RET_STATE_ERR_EMPTY;

	SharedMutex_t* pAccess	 = &((*pcHandle)->Access);
	RET_STATE_t rwLockResult = SharedMutex_ReadWriteLock(pAccess, waitMs, pLockKey);

	LOCAL_DEBUG_PRINT("LL rwLock status: %s", RetState_GetStr(rwLockResult));
	return rwLockResult;
}

/**
 * @brief Unlock LinkedList after RE_WR lock
 * 
 * @param[in] pcHandle handle for LL object
 * @retval RET_STATE_ERR_PARAM bad input parameter 
 * @retval RET_STATE_ERR_EMPTY LL isn't inited
 * @retval RET_STATE_ERR_BUSY LL locked on WR by other thread
 * @retval RET_STATE_SUCCESS LL unlocked from RE_WR successfuly
 */
RET_STATE_t LinkedList_ReadWriteUnlock(LinkedList_Handle_t* const pcHandle) {
	if (!pcHandle) {
		PANIC();
		return RET_STATE_ERR_PARAM;
	}

	if (!(*pcHandle))
		return RET_STATE_ERR_EMPTY;

	SharedMutex_t* pAccess	   = &((*pcHandle)->Access);
	RET_STATE_t rwUnlockResult = SharedMutex_ReadWriteUnlock(pAccess);

	LOCAL_DEBUG_PRINT("LL rwUnlock status: %s", RetState_GetStr(rwUnlockResult));
	return rwUnlockResult;
}

/**
 * @brief Interanl function for any LinkedList lock type check
 * 
 * @param[in] pcHandle handle for LL object
 * @param[in] extLockKey lock key for private access
 * @retval is locked/unlocked
 */
static bool LinkedList_IsLocked(LinkedList_Handle_t* const pcHandle, u32 extLockKey) {
	if (!pcHandle) {
		PANIC();
		return RET_STATE_ERR_PARAM;
	}

	if (!(*pcHandle))
		return RET_STATE_ERR_EMPTY;

	SharedMutex_t* pAccess = &((*pcHandle)->Access);

	bool isLocked = SharedMutex_IsLocked(pAccess, extLockKey);
	LOCAL_DEBUG_PRINT("LL lock status: %d", isLocked);
	return isLocked;
}

/**
 * @brief LinkedList object creation
 * 
 * @param[in] pcHandle handle for LL object
 * @param[in] fpGetMs func pointer to getMs realization
 * @param[in] fpWaitMs func pointer to waitMs realization
 * @retval RET_STATE_ERR_PARAM bad input parameter 
 * @retval RET_STATE_ERR_BUSY LL was created before
 * @retval RET_STATE_ERR_MEMORY some problems with memory allocation
 * @retval RET_STATE_SUCCESS LL created successfuly
 */
RET_STATE_t LinkedList_Create(LinkedList_Handle_t* const pcHandle, SharedMutex_GetMs_Fptr_t fpGetMs,
							  SharedMutex_WaitMs_Fptr_t fpWaitMs) {
	if (!pcHandle) {
		PANIC();
		return RET_STATE_ERR_PARAM;
	}

	/**
	 * Get and wait functions must be passed consistently both
	 */
	if ((fpGetMs == NULL && fpWaitMs != NULL) || (fpGetMs != NULL && fpWaitMs == NULL)) {
		LOCAL_DEBUG_PRINT("Callbacks consistency error!");
		PANIC();
		return RET_STATE_ERR_PARAM;
	}

	SYS_CRITICAL_ON();

	/**
	 * Prohibit recreation chance of the LinkedList object
	 */
	if (*pcHandle) {
		SYS_CRITICAL_OFF();
		return RET_STATE_ERR_BUSY;
	}

	/**
	 * Take a piece of memory for the main object
	 */
	LOCAL_DEBUG_PRINT("FREE HEAP: %d", LL_GET_FREE_HEAP());
	*pcHandle = (LinkedList_Handle_t)LL_MALLOC(sizeof(LinkedList_Object_t), __FILENAME__, __LINE__,
											   MEM_ALLOC_UNLIM_TMO);
	if (!(*pcHandle)) {
		SYS_CRITICAL_OFF();
		return RET_STATE_ERR_MEMORY;
	}

	LL_MEMSET((void*)*pcHandle, 0, sizeof(LinkedList_Object_t));
	SharedMutex_t* pAccess = &((*pcHandle)->Access);
	SharedMutex_Init(pAccess, fpGetMs, fpWaitMs);

	SYS_CRITICAL_OFF();
	LOCAL_DEBUG_PRINT("Created linkedList object 0x%08x on static addr 0x%08x", *pcHandle,
					  pcHandle);
	return RET_STATE_SUCCESS;
}

/**
 * @brief LinkedList object destruction
 * 
 * @param[in] pcHandle handle for LL object
 * @retval RET_STATE_ERR_PARAM bad input parameter 
 * @retval RET_STATE_ERR_EMPTY LL isn't inited
 * @retval RET_STATE_ERR_BUSY LL locked or nodes cnt within isn't zero
 * @retval RET_STATE_SUCCESS LL destructed successfuly
 */
RET_STATE_t LinkedList_Destruct(LinkedList_Handle_t* const pcHandle) {
	if (!pcHandle) {
		PANIC();
		return RET_STATE_ERR_PARAM;
	}

	if (!(*pcHandle))
		return RET_STATE_ERR_EMPTY;

	SYS_CRITICAL_ON();

	/**
	 * Stop object destruction if it's not empty inside
	 */
	if ((*pcHandle)->Metrics.NodesNum) {
		SYS_CRITICAL_OFF();
		return RET_STATE_ERR_BUSY;
	}

	LinkedList_Handle_t llObj = *pcHandle;

	/**
	 * Check if object mutex is free
	 */
	if (llObj->Access.LockType != SHARED_MUTEX_LOCK_NO) {
		SYS_CRITICAL_OFF();
		LOCAL_DEBUG_PRINT("Object is busy, lock type %d", llObj->Access.LockType);
		return RET_STATE_ERR_BUSY;
	}

	/**
	 * Destroy metadata and main object,
	 * print debug message and clear it finally
	 */
	if (llObj->Metadata.Addr)
		LL_FREE(llObj->Metadata.Addr);
	LL_FREE((void*)llObj);

	LOCAL_DEBUG_PRINT("Destroyed linkedList object 0x%08x on static addr 0x%08x", *pcHandle,
					  pcHandle);
	LOCAL_DEBUG_PRINT("FREE HEAP: %d", LL_GET_FREE_HEAP());

	*pcHandle = NULL;

	SYS_CRITICAL_OFF();
	return RET_STATE_SUCCESS;
}

static RET_STATE_t __LinkedList_Insert(LinkedList_Handle_t* const pcHandle, void* pData, u32 dSize,
									   u32 pos, u32 extLockKey) {
	if (!pcHandle || !pData) {
		PANIC();
		return RET_STATE_ERR_PARAM;
	}

	if (!(*pcHandle))
		return RET_STATE_ERR_EMPTY;

	SYS_CRITICAL_ON();

	LinkedList_Handle_t llObj = *pcHandle;

	if (LinkedList_IsLocked(&llObj, extLockKey)) {
		SYS_CRITICAL_OFF();
		return RET_STATE_ERR_BUSY;
	}

	/**
	 * Create one big node, which contains all required data
	 */
	u32 nodeSize = sizeof(LinkedList_Node_t) + dSize * sizeof(u8);
	LinkedList_Node_t* pNewNode =
		(LinkedList_Node_t*)LL_MALLOC(nodeSize, __FILENAME__, __LINE__, MEM_ALLOC_UNLIM_TMO);
	if (!pNewNode) {
		SYS_CRITICAL_OFF();
		return RET_STATE_ERR_MEMORY;
	}

	pNewNode->Next		= NULL;
	pNewNode->Data.Addr = (void*)((u8*)pNewNode + sizeof(LinkedList_Node_t));
	pNewNode->Data.Size = dSize;

	LOCAL_DEBUG_PRINT(
		"New node (%d bytes) created on addr 0x%08x; "
		"Data space (%d bytes) exists on addr 0x%08x",
		nodeSize, pNewNode, dSize, pNewNode->Data.Addr);

	LL_MEMCPY(pNewNode->Data.Addr, pData, dSize);

	/**
	 * Check if the rear node of LinkedList doesn't exist
	 * and rewrite it
	 */
	if (llObj->Rear == NULL) {
		llObj->Rear				= pNewNode;
		llObj->Metrics.NodesNum = 1;
		llObj->Metrics.BytesNum = dSize;
		LOCAL_DEBUG_PRINT("There is the first node in the LinkedList");

		SYS_CRITICAL_OFF();
		return RET_STATE_SUCCESS;
	}

	/**
	 * If new node is rear and rear is exists - 
	 * insert input node instead
	 */
	if (pos == LINKED_LIST_POS_REAR) {
		pNewNode->Next = llObj->Rear;
		llObj->Rear	   = pNewNode;
		llObj->Metrics.NodesNum++;
		llObj->Metrics.BytesNum += dSize;
		LOCAL_DEBUG_PRINT("Add new node on rear");

		SYS_CRITICAL_OFF();
		return RET_STATE_SUCCESS;
	}

	/**
	 * Try to find latest node with empy "Next" field
	 * If pos variable will be catched before it -
	 * insert input node on the pCurrNode place
	 */
	u32 nodeCnt					 = 0;
	LinkedList_Node_t* pCurrNode = llObj->Rear;
	while (pCurrNode->Next != NULL) {
		if (nodeCnt == pos - 1) {
			pNewNode->Next	= pCurrNode->Next;
			pCurrNode->Next = pNewNode;
			LOCAL_DEBUG_PRINT("Add new node on pos %d", pos);
			break;
		}

		pCurrNode = pCurrNode->Next;
		nodeCnt++;
	}

	/**
	 * If cycle upper has found the latest node - 
	 * add new node on the front of LinkedList
	 */
	if (pCurrNode->Next == NULL) {
		pCurrNode->Next = pNewNode;
		LOCAL_DEBUG_PRINT("Add new node on front");
	}

	llObj->Metrics.NodesNum++;
	llObj->Metrics.BytesNum += dSize;

	SYS_CRITICAL_OFF();
	return RET_STATE_SUCCESS;
}

/**
 * @brief Add new data in LinkedList on selected position
 * 
 * @param[in] pcHandle handle for LL object
 * @param[in] pData pointer to a data needs to be written
 * @param[in] dSize data size in bytes
 * @param[in] pos numeric position of node in LinkedList
 * @retval RET_STATE_ERR_PARAM bad input parameter 
 * @retval RET_STATE_ERR_EMPTY LL isn't inited
 * @retval RET_STATE_ERR_BUSY LL locked
 * @retval RET_STATE_ERR_MEMORY some problems with memory allocation
 * @retval RET_STATE_SUCCESS data inserted to LL successfuly
 */
RET_STATE_t LinkedList_Insert(LinkedList_Handle_t* const pcHandle, void* pData, u32 dSize,
							  u32 pos) {
	return __LinkedList_Insert(pcHandle, pData, dSize, pos, 0);
}

/**
 * @brief Add new data in LinkedList on selected position
 * 
 * @param[in] pcHandle handle for LL object
 * @param[in] pData pointer to a data needs to be written
 * @param[in] dSize data size in bytes
 * @param[in] pos numeric position of node in LinkedList
 * @param[in] extLockKey lock key for private access
 * @retval RET_STATE_ERR_PARAM bad input parameter 
 * @retval RET_STATE_ERR_EMPTY LL isn't inited
 * @retval RET_STATE_ERR_BUSY LL locked
 * @retval RET_STATE_ERR_MEMORY some problems with memory allocation
 * @retval RET_STATE_SUCCESS data inserted to LL successfuly
 */
RET_STATE_t LinkedList_PrivateInsert(LinkedList_Handle_t* const pcHandle, void* pData, u32 dSize,
									 u32 pos, u32 lockKey) {
	return __LinkedList_Insert(pcHandle, pData, dSize, pos, lockKey);
}

/**
 * @brief Internal function for node readout and memory cleanup
 * 
 * @param[in] pNodeToRm data pointer for LL node
 * @param[out] pExtData data pointer for external memory
 * @param[in] pExtDSize data size
 */
void static LinkedList_SaveDataCleanMem(LinkedList_Node_t* pNodeToRm, void* pExtData,
										u32* pExtDSize) {
	/**
	 * Here we save data from desired node in external pointers and
	 * cleanup memory resources
	 */
	if (pExtData)
		LL_MEMCPY(pExtData, pNodeToRm->Data.Addr, pNodeToRm->Data.Size);
	if (pExtDSize)
		*pExtDSize = pNodeToRm->Data.Size;

	LL_FREE((void*)pNodeToRm);
}

static RET_STATE_t __LinkedList_Extract(LinkedList_Handle_t* const pcHandle, void* pData,
										u32* pDSize, u32 pos, u32 extLockKey) {
	if (!pcHandle) {
		PANIC();
		return RET_STATE_ERR_PARAM;
	}

	if (!(*pcHandle))
		return RET_STATE_ERR_EMPTY;

	SYS_CRITICAL_ON();

	LinkedList_Handle_t llObj = *pcHandle;

	if (LinkedList_IsLocked(&llObj, extLockKey)) {
		SYS_CRITICAL_OFF();
		return RET_STATE_ERR_BUSY;
	}

	/**
	 * This is a quick checkup - do we have anything inside the LinkedList or
	 * not. If yes - it's no matter, which node position we need to extract, 
	 * cause each 'pos' value bigger then total number of nodes - it's the
	 * front node
	 */
	if (llObj->Metrics.NodesNum == 0) {
		LOCAL_DEBUG_PRINT("There are no elements in LinkedList");

		SYS_CRITICAL_OFF();
		return RET_STATE_ERR_EMPTY;
	}

	if (pos >= llObj->Metrics.NodesNum)
		pos = llObj->Metrics.NodesNum - 1;

	/**
	 * As we need to modify previous node in case of extracting current, 
	 * here we handle one exception from normal extracting flow
	 * below - when the front node needs to be removed (pos == 0)
	 */
	if (pos == LINKED_LIST_POS_REAR) {
		LinkedList_Node_t* nodeToRemove = llObj->Rear;
		llObj->Rear						= llObj->Rear->Next;

		LinkedList_SaveDataCleanMem(nodeToRemove, pData, pDSize);
		llObj->Metrics.NodesNum--;
		llObj->Metrics.BytesNum -= nodeToRemove->Data.Size;

		LOCAL_DEBUG_PRINT(
			"Remove node 0x%08x from pos %d; "
			"Rear node shifted to 0x%08x",
			nodeToRemove, pos, llObj->Rear);

		SYS_CRITICAL_OFF();
		return RET_STATE_SUCCESS;
	}

	/**
	 * Here is the main sycle of desired node search. And there are nothing
	 * unusual, but be carefull - nodeCnt must be (pos - 1) - cause we are on
	 * the previous step from the node to be removed
	 */
	u32 nodeCnt					 = 0;
	LinkedList_Node_t* pCurrNode = llObj->Rear;
	while (pCurrNode->Next != NULL) {
		if (nodeCnt == pos - 1) {
			LinkedList_Node_t* nodeToRemove = pCurrNode->Next;
			pCurrNode->Next					= pCurrNode->Next->Next;

			LinkedList_SaveDataCleanMem(nodeToRemove, pData, pDSize);
			llObj->Metrics.NodesNum--;
			llObj->Metrics.BytesNum -= nodeToRemove->Data.Size;

			LOCAL_DEBUG_PRINT(
				"Remove node 0x%08x from pos %d; "
				"Link between nodes created (0x%08x->0x%08x)",
				nodeToRemove, pos, pCurrNode, pCurrNode->Next);

			break;
		}

		pCurrNode = pCurrNode->Next;
		nodeCnt++;
	}

	SYS_CRITICAL_OFF();
	return RET_STATE_SUCCESS;
}

/**
 * @brief Extract data from LinkedList from selected position
 * 
 * @param[in] pcHandle handle for LL object
 * @param[out] pData (optional) pointer to a data needs to be extracted
 * @param[out] dSize (optional) pointer to a data size in bytes
 * @param[in] pos numeric position of node in LinkedList
 * @retval RET_STATE_ERR_PARAM bad input parameter 
 * @retval RET_STATE_ERR_EMPTY LL isn't inited or there are no node in LL
 * @retval RET_STATE_ERR_BUSY LL locked
 * @retval RET_STATE_SUCCESS data extracted from LL successfuly
 */
RET_STATE_t LinkedList_Extract(LinkedList_Handle_t* const pcHandle, void* pData, u32* pDSize,
							   u32 pos) {
	return __LinkedList_Extract(pcHandle, pData, pDSize, pos, 0);
}

/**
 * @brief Extract data from LinkedList from selected position
 * 
 * @param[in] pcHandle handle for LL object
 * @param[out] pData (optional) pointer to a data needs to be extracted
 * @param[out] dSize (optional) pointer to a data size in bytes
 * @param[in] pos numeric position of node in LinkedList
 * @param[in] extLockKey lock key for private access
 * @retval RET_STATE_ERR_PARAM bad input parameter 
 * @retval RET_STATE_ERR_EMPTY LL isn't inited or there are no nodes in LL
 * @retval RET_STATE_ERR_BUSY LL locked
 * @retval RET_STATE_SUCCESS data extracted from LL successfuly
 */
RET_STATE_t LinkedList_PrivateExtract(LinkedList_Handle_t* const pcHandle, void* pData, u32* pDSize,
									  u32 pos, u32 lockKey) {
	return __LinkedList_Extract(pcHandle, pData, pDSize, pos, lockKey);
}

/**
 * @brief Take out the data pointer from LinkedList without node extraction
 * 
 * @param[in] pcHandle handle for LL object
 * @param[out] pDataAddr (optional) pointer to an external data address
 * @param[out] pDataSizeAddr (optional) pointer to an external data size address
 * @param[in] pos numeric position of node in LL
 * @retval RET_STATE_ERR_PARAM bad input parameter 
 * @retval RET_STATE_ERR_EMPTY LL isn't inited or there are no nodes in LL
 * @retval RET_STATE_SUCCESS pointer got successfuly
 */
RET_STATE_t LinkedList_GetDataPtr(LinkedList_Handle_t* const pcHandle, void** pDataAddr,
								  u32** pDataSizeAddr, u32 pos) {
	if (!pcHandle) {
		PANIC();
		return RET_STATE_ERR_PARAM;
	}

	if (!(*pcHandle))
		return RET_STATE_ERR_EMPTY;

	SYS_CRITICAL_ON();

	LinkedList_Handle_t llObj = *pcHandle;

	if (llObj->Metrics.NodesNum == 0) {
		LOCAL_DEBUG_PRINT("There are no elements in LinkedList");

		SYS_CRITICAL_OFF();
		return RET_STATE_ERR_EMPTY;
	}

	if (pos >= llObj->Metrics.NodesNum)
		pos = llObj->Metrics.NodesNum - 1;

	u32 nodeCnt					 = 0;
	LinkedList_Node_t* pCurrNode = llObj->Rear;
	while (pCurrNode != NULL && pos != nodeCnt) {
		pCurrNode = pCurrNode->Next;
		nodeCnt++;
	}

	if (pDataAddr)
		*pDataAddr = pCurrNode->Data.Addr;
	if (pDataSizeAddr)
		*pDataSizeAddr = &pCurrNode->Data.Size;

	SYS_CRITICAL_OFF();
	return RET_STATE_SUCCESS;
}

/**
 * @brief Flush all nodes in LinkedList, clean up memory
 * 
 * @param[in] pcHandle handle for LL object
 * @retval Internally call of LinkedList_Extract
 * @retval RET_STATE_SUCCESS if there are no nodes in LL
 */
RET_STATE_t LinkedList_Flush(LinkedList_Handle_t* const pcHandle) {
	RET_STATE_t extrState = RET_STATE_UNDEF;

	/**
	 * Yes, just read out all data one-after-one, with
	 * extract() function. To speed up, we should always read
	 * zero (rear) node, while extract() will not 
	 * return ERR_EMPTY result.
	 */
	SYS_CRITICAL_ON();
	do {
		extrState = LinkedList_Extract(pcHandle, NULL, NULL, LINKED_LIST_POS_REAR);

		if (extrState != RET_STATE_ERR_EMPTY && extrState != RET_STATE_SUCCESS)
			break;

	} while (extrState != RET_STATE_ERR_EMPTY);
	SYS_CRITICAL_OFF();

	return extrState == RET_STATE_ERR_EMPTY ? RET_STATE_SUCCESS : extrState;
}

/**
 * @brief Reset metrics in LinkedList and go through it again
 * 
 * @param[in] pcHandle handle for LL object
 * @retval RET_STATE_ERR_PARAM bad input parameter 
 * @retval RET_STATE_ERR_EMPTY LL isn't inited
 * @retval RET_STATE_SUCCESS LL checkouted successfuly
 */
RET_STATE_t LinkedList_Checkout(LinkedList_Handle_t* const pcHandle) {
	if (!pcHandle) {
		PANIC();
		return RET_STATE_ERR_PARAM;
	}

	if (!(*pcHandle))
		return RET_STATE_ERR_EMPTY;

	SYS_CRITICAL_ON();

	LinkedList_Handle_t llObj = *pcHandle;

	llObj->Metrics.NodesNum = 0;
	llObj->Metrics.BytesNum = 0;

	LinkedList_Node_t* pCurrNode = llObj->Rear;
	while (pCurrNode != NULL) {
		LOCAL_DEBUG_PRINT("Node addr: 0x%08x, Cnt: %d, Bytes: %d, Data addr: 0x%08x",
						  (void*)pCurrNode, llObj->Metrics.NodesNum, llObj->Metrics.BytesNum,
						  pCurrNode->Data.Addr);

		llObj->Metrics.NodesNum++;
		llObj->Metrics.BytesNum += pCurrNode->Data.Size;

		pCurrNode = pCurrNode->Next;
	}

	LOCAL_DEBUG_PRINT("There are %d elements and %d data bytes in LinkedList",
					  llObj->Metrics.NodesNum, llObj->Metrics.BytesNum);

	SYS_CRITICAL_OFF();
	return RET_STATE_SUCCESS;
}

/**
 * @brief Get LinkedList metadata size
 * 
 * @param[in] pcHandle handle for LL object
 * @retval metadata size in bytes
 */
u32 LinkedList_GetMetadataSize(LinkedList_Handle_t* const pcHandle) {
	if (!pcHandle) {
		PANIC();
		return 0;
	}

	if (!(*pcHandle))
		return 0;

	return (*pcHandle)->Metadata.Size;
}

RET_STATE_t LinkedList_GetMetadata(LinkedList_Handle_t* const pcHandle, void* pMetadata,
								   u32 pMetadataMaxSize) {
	if (!pcHandle || !pMetadata) {
		PANIC();
		return RET_STATE_ERR_PARAM;
	}

	if (!(*pcHandle))
		return RET_STATE_ERR_EMPTY;

	SYS_CRITICAL_ON();

	LinkedList_Handle_t llObj = *pcHandle;

	if (llObj->Metadata.Size == 0 || llObj->Metadata.Addr == NULL) {
		SYS_CRITICAL_OFF();
		return RET_STATE_ERR_EMPTY;
	}

	if (llObj->Metadata.Size > pMetadataMaxSize) {
		SYS_CRITICAL_OFF();
		return RET_STATE_ERR_OVERFLOW;
	}

	LL_MEMCPY(pMetadata, llObj->Metadata.Addr, llObj->Metadata.Size);

	SYS_CRITICAL_OFF();
	return RET_STATE_SUCCESS;
}

static RET_STATE_t __LinkedList_UpdateMetadata(LinkedList_Handle_t* const pcHandle, void* pMetadata,
											   u32 metadataSize, u32 extLockKey) {
	if (!pcHandle || !pMetadata) {
		PANIC();
		return RET_STATE_ERR_PARAM;
	}

	if (!(*pcHandle))
		return RET_STATE_ERR_EMPTY;

	SYS_CRITICAL_ON();

	LinkedList_Handle_t llObj = *pcHandle;

	if (LinkedList_IsLocked(&llObj, extLockKey)) {
		SYS_CRITICAL_OFF();
		return RET_STATE_ERR_BUSY;
	}

	/**
	 * If metadata exists, but keeps anoter data - update it
	 */
	if (llObj->Metadata.Addr) {
		LL_FREE(llObj->Metadata.Addr);
		llObj->Metadata.Addr = NULL;
		llObj->Metadata.Size = 0;
	}

	/**
	 * Here metadata doesn't exist - try to allocate memory for it
	 */
	if (!llObj->Metadata.Addr) {
		llObj->Metadata.Addr =
			LL_MALLOC(metadataSize * sizeof(u8), __FILENAME__, __LINE__, MEM_ALLOC_UNLIM_TMO);
		if (!llObj->Metadata.Addr) {
			SYS_CRITICAL_OFF();
			return RET_STATE_ERR_MEMORY;
		}
	}

	LL_MEMCPY(llObj->Metadata.Addr, pMetadata, metadataSize);
	llObj->Metadata.Size = metadataSize;
	LOCAL_DEBUG_PRINT("Metadata created on addr: 0x%08x, Bytes: %d", llObj->Metadata.Addr,
					  metadataSize);

	SYS_CRITICAL_OFF();
	return RET_STATE_SUCCESS;
}

/**
 * @brief LinkedList metadata update
 * 
 * @param[in] pcHandle handle for LL object
 * @param[out] pMetadata pointer to data
 * @param[in] metadataSize data size
 * @retval RET_STATE_ERR_PARAM bad input parameter 
 * @retval RET_STATE_ERR_BUSY LL locked
 * @retval RET_STATE_ERR_MEMORY some problems with memory allocation
 * @retval RET_STATE_SUCCESS LL metadata updated successfuly
 */
RET_STATE_t LinkedList_UpdateMetadata(LinkedList_Handle_t* const pcHandle, void* pMetadata,
									  u32 size) {
	return __LinkedList_UpdateMetadata(pcHandle, pMetadata, size, 0);
}

/**
 * @brief LinkedList metadata update
 * 
 * @param[in] pcHandle handle for LL object
 * @param[out] pMetadata pointer to data
 * @param[in] metadataSize data size
 * @param[in] extLockKey lock key for private access
 * @retval RET_STATE_ERR_PARAM bad input parameter 
 * @retval RET_STATE_ERR_BUSY LL locked
 * @retval RET_STATE_ERR_MEMORY some problems with memory allocation
 * @retval RET_STATE_SUCCESS LL metadata updated successfuly
 */
RET_STATE_t LinkedList_PrivateUpdateMetadata(LinkedList_Handle_t* const pcHandle, void* pMetadata,
											 u32 size, u32 lockKey) {
	return __LinkedList_UpdateMetadata(pcHandle, pMetadata, size, lockKey);
}

/**
 * @brief Get nodes num in LinkedList 
 * 
 * @param[in] pcHandle handle for LL object
 * @retval number of nodes in LL
 */
u32 LinkedList_GetNodesNum(LinkedList_Handle_t* const pcHandle) {
	if (!pcHandle) {
		PANIC();
		return 0;
	}

	if (!(*pcHandle))
		return 0;

	return (*pcHandle)->Metrics.NodesNum;
}

/**
 * @brief Get total bytes num of data in LinkedList 
 * 
 * @param[in] pcHandle handle for LL object
 * @return number of data in LL in bytes
 */
u32 LinkedList_GetBytesNum(LinkedList_Handle_t* const pcHandle) {
	if (!pcHandle) {
		PANIC();
		return 0;
	}

	if (!(*pcHandle))
		return 0;

	return (*pcHandle)->Metrics.BytesNum;
}
