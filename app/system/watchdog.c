#include "watchdog.h"
#include "debug.h"
#include "platform.h"
#include "rtos_analyzer.h"

#if DEBUG_ENABLE
#define WATCHDOG_MAX_TIMEOUT_MS (DELAY_1_DAY)
#else /* DEBUG_ENABLE */
#define WATCHDOG_MAX_TIMEOUT_MS (3 * DELAY_1_MINUTE)
#endif /* DEBUG_ENABLE */

static TaskHandle_t WatchDog_Handle;
static TickType_t WatchDog_EndTs;

TaskHandle_t WatchDog_GetTaskHandle(void) {
	return WatchDog_Handle;
}

void WatchDog_Init(void) {
	Pl_Watchdog_Init();
}

void WatchDog_ResetCnt(void) {
	Pl_Watchdog_ReloadCounter();
	DEBUG_LOG_LVL_PRINT(LOG_LVL_TRACE, "WatchDog counter reloaded");
}

static void vTask_WatchDog_Process(void* pvParameters) {
	WatchDog_EndTs = xTaskGetTickCount() + (u32)(pvParameters);

	for (;;) {
		WatchDog_ResetCnt();

		vTaskDelay(5 * DELAY_1_SECOND);

		/**
		 * Delete watchdog task after timeout
		 * User needs to reset iwds himself 
		 */
		if (xTaskGetTickCount() > WatchDog_EndTs)
			RTOS_Analyzer_DeleteTask(&WatchDog_Handle);
	}
}

void WatchDog_TaskCreateIfNotExists(u32 tmo) {
	if (!WatchDog_Handle) {
		BaseType_t taskState =
			RTOS_Analyzer_CreateTask(vTask_WatchDog_Process, "watchdog-driver", WATCHDOG_TASK_STACK,
									 (void*)tmo, WATCHDOG_TASK_PRIORITY, &WatchDog_Handle);

		if (taskState != pdPASS) {
			DEBUG_LOG_LVL_PRINT(LOG_LVL_CRITIC, "WatchDog isn't created with err: %d", taskState);
			PANIC();
			// TODO EVENT
		}
	}
}

void WatchDog_TaskCreateOrProlongate(u32 tmo) {
	if (!WatchDog_Handle)
		WatchDog_TaskCreateIfNotExists(tmo);
	else
		WatchDog_EndTs += tmo;
}

void WatchDog_TaskDeleteIfExists(void) {
	if (WatchDog_Handle)
		RTOS_Analyzer_DeleteTask(&WatchDog_Handle);
}

void FreeRTOS_WatchDog_InitComponents(bool resources, bool tasks) {
	if (resources) {
	}

	if (tasks) {
		WatchDog_TaskCreateIfNotExists(WATCHDOG_MAX_TIMEOUT_MS);
	}
}
