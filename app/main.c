#include "main.h"
#include "bkp_storage.h"
#include "debug.h"
#include "delay.h"
#include "device_name.h"
#include "health_check.h"
#include "mem_wrapper.h"
#include "platform.h"
#include "rand.h"
#include "shell_root.h"
#include "storage.h"
#include "usb.h"
#include "watchdog.h"

void HardFault_Clbk(u32 pcVal) {
	BkpStorage_SetValue(BKP_KEY_SYS_FAULT_EXEPTION_ADDR, pcVal);
}

void ErrorHandler(char* pFile, int line) {
	DISCARD_UNUSED(pFile);
	DISCARD_UNUSED(line);

	PL_IrqOff();
	PL_SET_BKPT();
	while (true) {
	}
}

void vApplicationStackOverflowHook(TaskHandle_t xTask, char* pcTaskName) {
	ErrorHandler(pcTaskName, 0);
}

void FreeRTOS_InitComponents(bool resources, bool tasks) {
	FreeRTOS_WatchDog_InitComponents(resources, tasks);
	FreeRTOS_DebugSend_InitComponents(resources, tasks);
	FreeRTOS_ShellRoot_InitComponents(resources, tasks);
	FreeRTOS_HealthCheck_InitComponents(resources, tasks);
}

int main(void) {
#if !DEBUG_ENABLE
	WatchDog_Init();
#endif /* !DEBUG_ENABLE */

	MemWrap_AllocTracker_Init();
	FreeRTOS_InitComponents(true, false);

#if DEBUG_ENABLE
	Pl_Sys_DebugInit();
#endif /* DEBUG_ENABLE */
	Pl_Init(HardFault_Clbk, configLIBRARY_MAX_SYSCALL_INTERRUPT_PRIORITY);
	Pl_LedDebug_Init();
	Pl_Crc_Init();
	Pl_SysCpuCnt_Init();
	Delay_Init();
	Rand_Init();
	BkpStorage_Init();

	DeviceName_Generate();
	Debug_Init();
	Debug_PrintMainInfo();
	Debug_PrintSysInfo();
	// Storage_Init();

	FreeRTOS_InitComponents(false, true);
	vTaskStartScheduler();
	for (;;) {
		PANIC();
	}

	return 0;
}
