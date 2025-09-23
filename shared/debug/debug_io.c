

#include "dbg_cfg.h"
#include "debug.h"
#include "platform.h"

#define DEBUG_CHUNK_SIZE  128
#define DEBUG_RX_BUFF_LEN 128
static u8 Debug_RxBuff[DEBUG_RX_BUFF_LEN];

#if DBG_USE_RTOS
#include "mem_wrapper.h"
#include "rtos_analyzer.h"

static TaskHandle_t DebugSend_Handle;
static QueueHandle_t DebugSend_Queue;

static QueueHandle_t DebugRxSymbol_Queue;
static SemaphoreHandle_t DebugRxSequence_Mutex;
#endif /* DBG_USE_RTOS */

/*
 * Wrapper for printf() function native using via selected interface
 */
int _write(int fd, char* ptr, int len) {
#if DBG_USE_RTOS
	if (!DebugSend_Queue)
		return len;

	char* pBuff = MemWrap_Malloc(len, __FILENAME__, __LINE__, MEM_ALLOC_MAX_TMO);
	if (!pBuff)
		return len;

	memcpy((void*)pBuff, (void*)ptr, len);
	DebugMsg_t msg = {
		.Ptr = pBuff,
		.Len = len,
	};
	xQueueSend(DebugSend_Queue, (void*)&msg, 0);
#else  /* DBG_USE_RTOS */
	Debug_TransmitBuff(ptr, len);
#endif /* DBG_USE_RTOS */

	return len;
}

static void Debug_RxClbkUart(void* pVal) {
#if DBG_USE_RTOS
	BaseType_t xHigherPriorityTaskWoken = pdFALSE;
#endif /* DBG_USE_RTOS */

	char ch = *(char*)pVal;
#if DBG_USE_RTOS
	Debug_SendCharFromISR(ch, &xHigherPriorityTaskWoken);
#else  /* DBG_USE_RTOS */
	//
#endif /* DBG_USE_RTOS */

#if DBG_USE_RTOS
	if (xHigherPriorityTaskWoken == pdTRUE)
		portEND_SWITCHING_ISR(xHigherPriorityTaskWoken);
#endif /* DBG_USE_RTOS */
}

static void Debug_RxClbkUsb(u8* pBuff, u32 len) {
#if DBG_USE_RTOS
	BaseType_t xHigherPriorityTaskWoken = pdFALSE;
#endif /* DBG_USE_RTOS */

	for (u32 i = 0; i < len; i++) {
		char ch = *(char*)&pBuff[i];
#if DBG_USE_RTOS
		Debug_SendCharFromISR(ch, &xHigherPriorityTaskWoken);
#else  /* DBG_USE_RTOS */
		//
#endif /* DBG_USE_RTOS */
	}

#if DBG_USE_RTOS
	if (xHigherPriorityTaskWoken == pdTRUE)
		portEND_SWITCHING_ISR(xHigherPriorityTaskWoken);
#endif /* DBG_USE_RTOS */
}

void Debug_Init(void) {
	// setvbuf(stdin, NULL, _IONBF, 0);
	setvbuf(stdout, NULL, _IOFBF, 0);
	// setvbuf(stderr, NULL, _IONBF, 0);

#if defined DEBUG_OUTPUT_THROUGH_UART
	Pl_DebugUart_Init(DEBUG_UART_BAUDRATE, Debug_RxBuff, sizeof(Debug_RxBuff), Debug_RxClbkUart);
#elif defined DEBUG_OUTPUT_THROUGH_USB
	Pl_USB_CDC_Init(Debug_RxClbkUsb, Debug_RxBuff);
#endif
}

bool Debug_HardwareIsInit(void) {
#if defined DEBUG_OUTPUT_THROUGH_UART
	return Pl_IsInit.SerialDebug;
#elif defined DEBUG_OUTPUT_THROUGH_USB
	return Pl_USB_CDC_IsReady();
#endif
	return false;
}

bool Debug_SendChar(char ch, u32 waitTmo) {
#if DBG_USE_RTOS
	// Need mutex here in case of ShellRoot_SendCommand simultaneous using
	if (xSemaphoreTake(DebugRxSequence_Mutex, waitTmo) != pdTRUE)
		return false;
#endif /* DBG_USE_RTOS */

#if DBG_USE_RTOS
	bool ret = (bool)(xQueueSend(DebugRxSymbol_Queue, &ch, waitTmo) == pdPASS);
	xSemaphoreGive(DebugRxSequence_Mutex);
#else  /* DBG_USE_RTOS */
	// bool ret = false;
#endif /* DBG_USE_RTOS */

	return ret;
}

bool Debug_SendCharFromISR(char ch, BaseType_t* pWoken) {
#if DBG_USE_RTOS
	// Need mutex here in case of ShellRoot_SendCommand simultaneous using
	if (xSemaphoreTakeFromISR(DebugRxSequence_Mutex, pWoken) != pdTRUE)
		return false;
#endif /* DBG_USE_RTOS */

#if DBG_USE_RTOS
	bool ret = (bool)(xQueueSendFromISR(DebugRxSymbol_Queue, &ch, pWoken) == pdPASS);
	xSemaphoreGiveFromISR(DebugRxSequence_Mutex, pWoken);
#else  /* DBG_USE_RTOS */
	// bool ret = false;
#endif /* DBG_USE_RTOS */

	return ret;
}

bool Debug_SendString(char* pStr, u32 waitTmo) {
	ASSERT_CHECK(pStr);

	if (!pStr)
		return false;

#if DBG_USE_RTOS
	if (xSemaphoreTake(DebugRxSequence_Mutex, waitTmo) != pdTRUE)
		return false;

	u32 strLen = strlen(pStr);
	if (strLen >= sizeof(Debug_RxBuff)) {
		xSemaphoreGive(DebugRxSequence_Mutex);
		PANIC();
		return false;
	}

	for (u32 i = 0; i < strLen; i++) {
		if (xQueueSend(DebugRxSymbol_Queue, &pStr[i], waitTmo) != pdTRUE) {
			xSemaphoreGive(DebugRxSequence_Mutex);
			PANIC();
			return false;
		}
	}

	xSemaphoreGive(DebugRxSequence_Mutex);
#else  /* DBG_USE_RTOS */
	// return false;
#endif /* DBG_USE_RTOS */

	return true;
}

bool Debug_TransmitBuff(char* pBuff, u32 size) {
	bool isSent = false;

#if defined DEBUG_OUTPUT_THROUGH_UART
	isSent = Pl_DebugUart_SendBuff((u8*)pBuff, size);
#elif defined DEBUG_OUTPUT_THROUGH_USB
	u32 cnt = 0;

	while (true) {
		RET_STATE_t retState = Pl_USB_CDC_Transmit(pBuff, size);
		if (retState == RET_STATE_SUCCESS) {
			isSent = true;
			break;
		}

		PL_DELAY_MS(1);	 //TODO change to SYS_DELAY_MS
		//If USB isn't connected, all debug prints will be 10ms len
		if (cnt++ > 10)
			break;
	}
#endif /* DEBUG_OUTPUT_THROUGH_XXX */

	return isSent;
}

char Debug_ReceiveSymbol(u32 delay) {
	char symbol = 0;

#if DBG_USE_RTOS
	if (DebugRxSymbol_Queue)
		xQueueReceive(DebugRxSymbol_Queue, &symbol, delay);
#else  /* DBG_USE_RTOS */
	//
#endif /* DBG_USE_RTOS */

	return symbol;
}

#if DBG_USE_RTOS

TaskHandle_t DebugSend_GetTaskHandle(void) {
	return DebugSend_Handle;
}

static void vTask_DebugSend_Process(void* pvParameters) {
	DebugMsg_t msg;
	vTaskDelay(DELAY_1_SECOND * 2);

	Debug_PrintMainInfo();
	Debug_PrintSysInfo();

	for (;;) {
		xQueueReceive(DebugSend_Queue, &msg, portMAX_DELAY);
		vTaskPrioritySet(NULL, MAX_TASK_PRIORITY);

		// while (true) {
		// 	Debug_TransmitBuff(msg.Ptr, msg.Len);
		// 	MemWrap_Free(msg.Ptr);
		// 	if (uxQueueMessagesWaiting(DebugSend_Queue) > 0)
		// 		xQueueReceive(DebugSend_Queue, &msg, 0);
		// 	else
		// 		break;
		// }

		while (true) {
			s32 totalLen = msg.Len;
			char* pData	 = msg.Ptr;

			while (totalLen > 0) {
				u32 chunkLen = (totalLen > DEBUG_CHUNK_SIZE) ? DEBUG_CHUNK_SIZE : totalLen;
				Debug_TransmitBuff(pData, chunkLen);
				pData += chunkLen;
				totalLen -= chunkLen;
			}

			MemWrap_Free(msg.Ptr);

			if (uxQueueMessagesWaiting(DebugSend_Queue) > 0)
				xQueueReceive(DebugSend_Queue, &msg, 0);
			else
				break;
		}

		vTaskPrioritySet(NULL, DEBUG_SEND_TASK_PRIORITY);

/**
 * No any delays while debug in progress,
 * but add min delay in production code
 */
#if !DEBUG_ENABLE
/**
 * But disable delay if we are in tests mode
 */
#if !TEST_MODE_ENABLE
		vTaskDelay(5);
#endif /* !TEST_MODE_ENABLE */
#endif /* !DEBUG_ENABLE */
	}
}

void FreeRTOS_DebugSend_InitComponents(bool resources, bool tasks) {
	if (resources) {
		DebugSend_Queue = xQueueCreate(100, sizeof(DebugMsg_t));

		DebugRxSymbol_Queue	  = xQueueCreate(sizeof(Debug_RxBuff) / 3, sizeof(char));
		DebugRxSequence_Mutex = xSemaphoreCreateMutex();
		xSemaphoreGive(DebugRxSequence_Mutex);
	}

	if (tasks) {
		RTOS_Analyzer_CreateTask(vTask_DebugSend_Process, "debug-out-driver", DEBUG_SEND_TASK_STACK,
								 NULL, DEBUG_SEND_TASK_PRIORITY, &DebugSend_Handle);
	}
}

#else /* DBG_USE_RTOS */

TaskHandle_t DebugSend_GetTaskHandle(void) {
	return NULL;
}

void FreeRTOS_DebugSend_InitComponents(bool resources, bool tasks) {
	if (resources) {
	}

	if (tasks) {
	}
}

#endif /* DBG_USE_RTOS */
