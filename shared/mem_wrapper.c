#include "mem_wrapper.h"
#include "FreeRTOSConfig.h"
#include "debug.h"
#include "platform.h"

#if DEBUG_ENABLE
#define LOCAL_DEBUG_PRINT_ENABLE 0
#define LOCAL_DEBUG_TEST_ENABLE	 0
#endif /* DEBUG_ENABLE */

#if LOCAL_DEBUG_PRINT_ENABLE
#warning LOCAL_DEBUG_PRINT_ENABLE
#define LOCAL_DEBUG_PRINT DEBUG_PRINT_DIRECT_NL
#else /* LOCAL_DEBUG_PRINT_ENABLE */
#define LOCAL_DEBUG_PRINT(_f_, ...)
#endif /* LOCAL_DEBUG_PRINT_ENABLE */

#if LOCAL_DEBUG_TEST_ENABLE
#warning LOCAL_DEBUG_TEST_ENABLE
#endif /* LOCAL_DEBUG_TEST_ENABLE */

u8 PL_QUICKACCESS_DATA ucHeap[configTOTAL_HEAP_SIZE];  //TODO check RAM perf

bool MemWrap_IsAllocatedFromHeap(void* pAddr) {
	if (pAddr >= (void*)&ucHeap[0] && pAddr < (void*)&ucHeap[configTOTAL_HEAP_SIZE - 1])
		return true;

	return false;
}

#if MEM_ALLOC_TRACKER

#else /* MEM_ALLOC_TRACKER */

void MemWrap_AllocTracker_Init(void) {
}

void* MemWrap_Malloc(size_t size, char* pFile, u32 line, u32 timeoutMs) {
	void* pAddr = pvPortMalloc(size);
	LOCAL_DEBUG_PRINT("Malloc: %d, %s, %d, 0x%08x", size, pFile, line, pAddr);
	return pAddr;
}

void MemWrap_Free(void* pAddr) {
	vPortFree(pAddr);
	LOCAL_DEBUG_PRINT("Free: 0x%08x", pAddr);
}

u32 MemWrap_GetFreeHeapSize(void) {
	return xPortGetFreeHeapSize();
}

#endif /* MEM_ALLOC_TRACKER */
