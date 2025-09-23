#ifndef __MEM_WRAPPER_H
#define __MEM_WRAPPER_H

#include "main.h"

// TODO add usage

#define MEM_ALLOC_MAX_TMO	(10 * DELAY_1_MINUTE)
#define MEM_ALLOC_DEF_TMO	(5 * DELAY_1_MINUTE)
#define MEM_ALLOC_UNLIM_TMO (portMAX_DELAY)

bool MemWrap_IsAllocatedFromHeap(void* pAddr);
void MemWrap_AllocTracker_Init(void);
void* MemWrap_Malloc(size_t size, char* pFile, u32 line, u32 timeoutMs);
void MemWrap_Free(void* pAddr);
u32 MemWrap_GetFreeHeapSize(void);

#endif /* __MEM_WRAPPER_H */
