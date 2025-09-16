#ifndef __LINKED_LIST_H
#define __LINKED_LIST_H

#include "main.h"
#include "shared_mutex.h"

#define LINKED_LIST_POS_FRONT (~0U)
#define LINKED_LIST_POS_REAR  (0)

#define LINKED_LIST_AWAIT_UNLOCK_MS (2)

typedef struct {
	void* Addr;
	u32 Size;
} LinkedList_Data_t;

typedef struct LinkedList_Node {
	struct LinkedList_Node* Next;
	LinkedList_Data_t Data;
} LinkedList_Node_t;

typedef struct {
	u32 NodesNum;
	u32 BytesNum;
} LinkedList_Metrics_t;

typedef struct {
	LinkedList_Node_t* Rear;
	LinkedList_Data_t Metadata;
	LinkedList_Metrics_t Metrics;
	SharedMutex_t Access;
} LinkedList_Object_t;

typedef LinkedList_Object_t* LinkedList_Handle_t;

RET_STATE_t LinkedList_WriteLock(LinkedList_Handle_t* const pcHandle, u32 waitMs);
RET_STATE_t LinkedList_WriteUnlock(LinkedList_Handle_t* const pcHandle);
u32 LinkedList_GetConcurentReaders(LinkedList_Handle_t* const pcHandle);

RET_STATE_t LinkedList_ReadWriteLock(LinkedList_Handle_t* const pcHandle, u32 waitMs,
									 u32* pLockKey);
RET_STATE_t LinkedList_ReadWriteUnlock(LinkedList_Handle_t* const pcHandle);

RET_STATE_t LinkedList_Create(LinkedList_Handle_t* const pcHandle, SharedMutex_GetMs_Fptr_t fpGetMs,
							  SharedMutex_WaitMs_Fptr_t fpWaitMs);
RET_STATE_t LinkedList_Destruct(LinkedList_Handle_t* const pcHandle);

RET_STATE_t LinkedList_Insert(LinkedList_Handle_t* const pcHandle, void* pData, u32 dSize, u32 pos);
RET_STATE_t LinkedList_PrivateInsert(LinkedList_Handle_t* const pcHandle, void* pData, u32 dSize,
									 u32 pos, u32 lockKey);
RET_STATE_t LinkedList_Extract(LinkedList_Handle_t* const pcHandle, void* pData, u32* pDSize,
							   u32 pos);
RET_STATE_t LinkedList_PrivateExtract(LinkedList_Handle_t* const pcHandle, void* pData, u32* pDSize,
									  u32 pos, u32 lockKey);

RET_STATE_t LinkedList_GetDataPtr(LinkedList_Handle_t* const pcHandle, void** pDataAddr,
								  u32** pDataSizeAddr, u32 pos);

RET_STATE_t LinkedList_Flush(LinkedList_Handle_t* const pcHandle);
RET_STATE_t LinkedList_Checkout(LinkedList_Handle_t* const pcHandle);

u32 LinkedList_GetMetadataSize(LinkedList_Handle_t* const pcHandle);
RET_STATE_t LinkedList_GetMetadata(LinkedList_Handle_t* const pcHandle, void* pMetadata,
								   u32 pMetadataMaxSize);
RET_STATE_t LinkedList_UpdateMetadata(LinkedList_Handle_t* const pcHandle, void* pMetadata,
									  u32 size);
RET_STATE_t LinkedList_PrivateUpdateMetadata(LinkedList_Handle_t* const pcHandle, void* pMetadata,
											 u32 size, u32 lockKey);

u32 LinkedList_GetNodesNum(LinkedList_Handle_t* const pcHandle);
u32 LinkedList_GetBytesNum(LinkedList_Handle_t* const pcHandle);

#endif /* __LINKED_LIST_H */
