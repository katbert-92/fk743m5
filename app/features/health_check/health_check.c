#include "health_check.h"
#include "debug.h"
#include "health_check_cfg.h"
#include "rtos_analyzer.h"

#if HEALTH_CHECK

#if DEBUG_ENABLE
#define LOCAL_DEBUG_PRINT_ENABLE 0
#endif /* DEBUG_ENABLE */

#if LOCAL_DEBUG_PRINT_ENABLE
#warning LOCAL_DEBUG_PRINT_ENABLE
#define LOCAL_DEBUG_PRINT DEBUG_LOG_PRINT
#else /* DEBUG_ENABLE */
#define LOCAL_DEBUG_PRINT(_f_, ...)
#endif /* DEBUG_ENABLE */

static TaskHandle_t HealthCheck_Handle;

static void vTask_HealthCheck_Process(void* pvParameters) {
	RTOS_Analyzer_AddSystemTasksToRegistry();

	for (;;) {

		RTOS_Analyzer_Check();

		Debug_LedToggle();

		vTaskDelay(DELAY_1_SECOND / 10);
	}
}

void FreeRTOS_HealthCheck_InitComponents(bool resources, bool tasks) {
	if (resources) {
	}

	if (tasks) {
		RTOS_Analyzer_CreateTask(vTask_HealthCheck_Process, "health-check-process",
								 HEALTH_CHECK_TASK_STACK, NULL, HEALTH_CHECK_TASK_PRIORITY,
								 &HealthCheck_Handle);
	}
}

#else /* HEALTH_CHECK */

void FreeRTOS_HealthCheck_InitComponents(bool resources, bool tasks) {
}

#endif /* HEALTH_CHECK */
