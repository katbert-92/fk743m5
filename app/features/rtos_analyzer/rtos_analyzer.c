#include "rtos_analyzer.h"
#include "debug.h"
#include "rtos_analyzer_cfg.h"

#if DEBUG_ENABLE
#define LOCAL_DEBUG_PRINT_ENABLE 0
#endif /* DEBUG_ENABLE */

#if LOCAL_DEBUG_PRINT_ENABLE
#warning LOCAL_DEBUG_PRINT_ENABLE
#define LOCAL_DEBUG_PRINT DEBUG_LOG_PRINT
#else /* DEBUG_ENABLE */
#define LOCAL_DEBUG_PRINT(_f_, ...)
#endif /* DEBUG_ENABLE */

#if RTOS_ANALYZER

#include "mem_wrapper.h"
#include "shell_root.h"
#include "stringlib.h"
#include "watchdog.h"
#include "wsh_shell.h"

static TickType_t RTOS_Analyzer_StartTickCount = RTOS_ANALYZER_STARTUP_DELAY;
static TaskHandle_t TimerTask_Handle;
static TaskHandle_t IDLE_Task_Handle;

typedef struct {
	TaskHandle_t Handle;
	u32 StackInitBytes;
	bool RecoverAfterSuspend;
} TaskInfo_t;

typedef struct {
	u32 CurrNum;
	u32 Added;
	u32 Removed;
} RTOS_Analyzer_Metrics_t;

static TaskInfo_t TasksRegistry[RTOS_ANALYZER_TASKS_MAX_NUM];
static RTOS_Analyzer_Metrics_t TasksRegistryMetrics;

static s32 find_task_handle_idx(TaskHandle_t* pTaskHandle) {
	s32 retTaskIdx = -1;
	for (s32 taskIdx = 0; taskIdx < NUM_ELEMENTS(TasksRegistry); taskIdx++) {
		if (TasksRegistry[taskIdx].Handle == *pTaskHandle) {
			retTaskIdx = taskIdx;
			break;
		}
	}

	LOCAL_DEBUG_PRINT("task %sfound, idx: %d", retTaskIdx >= 0 ? "" : "not ", retTaskIdx);
	return retTaskIdx;
}

static s32 find_free_cell(void) {
	s32 retTaskIdx = -1;
	for (u32 taskIdx = 0; taskIdx < NUM_ELEMENTS(TasksRegistry); taskIdx++) {
		if (!TasksRegistry[taskIdx].Handle) {
			retTaskIdx = taskIdx;
			break;
		}
	}

	LOCAL_DEBUG_PRINT("free cell %sfound, idx: %d", retTaskIdx >= 0 ? "" : "not ", retTaskIdx);
	return retTaskIdx;
}

static void RTOS_Analyzer_AddTaskToRegistry(TaskHandle_t* pTaskHandle, u32 taskStackInitBytes) {
	ASSERT_CHECK(pTaskHandle != NULL);
	ASSERT_CHECK(TasksRegistryMetrics.CurrNum < NUM_ELEMENTS(TasksRegistry) - 1);

	if (!pTaskHandle)
		return;

	ASSERT_CHECK(*pTaskHandle != NULL);

	if (!*pTaskHandle)
		return;

	s32 taskIdx = find_task_handle_idx(pTaskHandle);
	if (taskIdx >= 0)
		return;

	taskIdx = find_free_cell();
	if (taskIdx < 0)
		return;

	SYS_CRITICAL_ON();
	TasksRegistry[taskIdx].Handle			   = *pTaskHandle;
	TasksRegistry[taskIdx].StackInitBytes	   = taskStackInitBytes * sizeof(u32);
	TasksRegistry[taskIdx].RecoverAfterSuspend = false;
	TasksRegistryMetrics.CurrNum++;
	TasksRegistryMetrics.Added++;
	SYS_CRITICAL_OFF();
}

static void RTOS_Analyzer_RemoveTaskFromRegistry(TaskHandle_t* pTaskHandle) {
	ASSERT_CHECK(pTaskHandle != NULL);

	if (!pTaskHandle)
		return;

	ASSERT_CHECK(*pTaskHandle != NULL);

	if (!*pTaskHandle)
		return;

	s32 taskIdx = find_task_handle_idx(pTaskHandle);
	if (taskIdx < 0)
		return;

	SYS_CRITICAL_ON();
	TasksRegistry[taskIdx].Handle			   = NULL;
	TasksRegistry[taskIdx].StackInitBytes	   = 0;
	TasksRegistry[taskIdx].RecoverAfterSuspend = false;
	TasksRegistryMetrics.CurrNum--;
	TasksRegistryMetrics.Removed++;
	SYS_CRITICAL_OFF();
}

BaseType_t RTOS_Analyzer_CreateTask(TaskFunction_t taskCode, const char* const pName,
									configSTACK_DEPTH_TYPE stackDepth, void* pParameters,
									UBaseType_t priority, TaskHandle_t* pTaskHandle) {
	if (*pTaskHandle != NULL) {
		PANIC();
		return pdFAIL;
	}

	BaseType_t createState =
		xTaskCreate(taskCode, pName, stackDepth, pParameters, priority, pTaskHandle);
	if (createState == pdPASS)
		RTOS_Analyzer_AddTaskToRegistry(pTaskHandle, stackDepth);

	ASSERT_CHECK(*pTaskHandle != NULL);
	return createState;
}

void RTOS_Analyzer_DeleteTask(TaskHandle_t* pTaskHandle) {
	TaskHandle_t tmpTaskHandle = *pTaskHandle;
	RTOS_Analyzer_RemoveTaskFromRegistry(pTaskHandle);
	*pTaskHandle = NULL;
	vTaskDelete(tmpTaskHandle);
}

void RTOS_Analyzer_AddSystemTasksToRegistry(void) {
	TimerTask_Handle = xTimerGetTimerDaemonTaskHandle();
	IDLE_Task_Handle = xTaskGetIdleTaskHandle();
	RTOS_Analyzer_AddTaskToRegistry(&TimerTask_Handle, configTIMER_TASK_STACK_DEPTH);
	RTOS_Analyzer_AddTaskToRegistry(&IDLE_Task_Handle, configMINIMAL_STACK_SIZE);
}

void RTOS_Analyzer_Check(void) {
	TickType_t now = xTaskGetTickCount();
	if (RTOS_Analyzer_StartTickCount > now)
		return;

	RTOS_Analyzer_StartTickCount = now + RTOS_ANALYZER_WORK_DELAY;
	for (u32 taskIdx = 0; taskIdx < NUM_ELEMENTS(TasksRegistry); taskIdx++) {
		if (!TasksRegistry[taskIdx].Handle)
			continue;

		if (eTaskGetState(TasksRegistry[taskIdx].Handle) == eDeleted ||
			eTaskGetState(TasksRegistry[taskIdx].Handle) == eInvalid)
			continue;

		u32 stackHwmBytes =
			sizeof(u32) * uxTaskGetStackHighWaterMark(TasksRegistry[taskIdx].Handle);
		if (stackHwmBytes < RTOS_ANALYZER_MIN_TASK_STACK_SIZE_TRIG) {
			char taskName[configMAX_TASK_NAME_LEN];
			snprintf(taskName, configMAX_TASK_NAME_LEN, "%s",
					 pcTaskGetName(TasksRegistry[taskIdx].Handle));

			DEBUG_LOG_LVL_PRINT(LOG_LVL_CRITIC, "Task '%s', stack HWM: %d bytes", taskName,
								stackHwmBytes);
			PANIC();
			//TODO EVENT

			RTOS_Analyzer_StartTickCount = portMAX_DELAY;
			break;
		}
	}

	u32 everMinHeapSize = xPortGetMinimumEverFreeHeapSize();
	if (everMinHeapSize <= RTOS_ANALYZER_MIN_HEAP_SIZE_TRIG) {
		DEBUG_LOG_LVL_PRINT(LOG_LVL_CRITIC, "Min ever free heap size: %d", everMinHeapSize);
		PANIC();
		//TODO EVENT

		RTOS_Analyzer_StartTickCount = portMAX_DELAY;
	}
}

// clang-format off
#define CMD_RTOS_OPT_TABLE() \
X_ENTRY(CMD_RTOS_OPT_HELP, WSH_SHELL_OPT_HELP()) \
X_ENTRY(CMD_RTOS_OPT_DEF, WSH_SHELL_OPT_NO(WSH_SHELL_OPT_ACCESS_READ)) \
X_ENTRY(CMD_RTOS_OPT_INFO, WSH_SHELL_OPT_WO_PARAM(WSH_SHELL_OPT_ACCESS_READ, "-i", "--info", "Get info about FreeRTOS and memory, JSON")) \
X_ENTRY(CMD_RTOS_OPT_SUSPEND, WSH_SHELL_OPT_WO_PARAM(WSH_SHELL_OPT_ACCESS_EXECUTE, "-s", "--suspend", "Suspend all tasks (excluding shell, debug and watchdog)")) \
X_ENTRY(CMD_RTOS_OPT_RESUME, WSH_SHELL_OPT_WO_PARAM(WSH_SHELL_OPT_ACCESS_EXECUTE, "-r", "--resume", "Resume all tasks")) \
X_ENTRY(CMD_RTOS_OPT_END, WSH_SHELL_OPT_END())
// clang-format on

#define X_ENTRY(en, m) en,
typedef enum { CMD_RTOS_OPT_TABLE() CMD_RTOS_OPT_ENUM_SIZE } CMD_RTOS_OPT_t;
#undef X_ENTRY

#define X_ENTRY(enum, opt) {enum, opt},
WshShellOption_t RtosOptArr[] = {CMD_RTOS_OPT_TABLE()};
#undef X_ENTRY

static WSH_SHELL_RET_STATE_t shell_cmd_rtos(const WshShellCmd_t* pcCmd, WshShell_Size_t argc,
											const char* pArgv[], void* pCtx) {
	if ((argc > 0 && pArgv == NULL) || pcCmd == NULL)
		return WSH_SHELL_RET_STATE_ERROR;

	char infoBuff[RTOS_ANALYZER_SHELL_BUFF_SIZE]	= "";
	char prettyPrint[RTOS_ANALYZER_SHELL_BUFF_SIZE] = "";
	u32 n											= 0;
	n = sprintf(infoBuff + n, JSON_FIELD_FIRST, "cmd", pcCmd->Name);

	HeapStats_t heapStats;
	vPortGetHeapStats(&heapStats);

	WshShell_Size_t tokenPos = 0;
	while (tokenPos < argc) {
		WshShellOption_Context_t optCtx = WshShellCmd_ParseOpt(pcCmd, argc, pArgv, &tokenPos);
		if (optCtx.Option == NULL)
			return WSH_SHELL_RET_STATE_ERR_EMPTY;

		switch (optCtx.Option->ID) {
			case CMD_RTOS_OPT_HELP:
				WshShellCmd_PrintOptionsOverview(pcCmd);
				break;

			case CMD_RTOS_OPT_DEF: {
				WSH_SHELL_PRINT("Tasks num: %d\r\n", uxTaskGetNumberOfTasks());
				float heapOnePcnt = (float)configTOTAL_HEAP_SIZE / 100.0f;
				WSH_SHELL_PRINT("Available heap space: %d bytes (%4.2f%% of memory used)\r\n",
								heapStats.xAvailableHeapSpaceInBytes,
								100.0f - (float)heapStats.xAvailableHeapSpaceInBytes / heapOnePcnt);
				WSH_SHELL_PRINT("Min ever free remaining: %d bytes (%4.2f%% of memory)\r\n",
								heapStats.xMinimumEverFreeBytesRemaining,
								100.0f -
									(float)heapStats.xMinimumEverFreeBytesRemaining / heapOnePcnt);
				WSH_SHELL_PRINT("Largest/smallest free block: %d / %d bytes\r\n",
								heapStats.xSizeOfLargestFreeBlockInBytes,
								heapStats.xSizeOfSmallestFreeBlockInBytes);
				WSH_SHELL_PRINT(
					"Successful allocations/frees: %d / %d, delta: %d\r\n",
					heapStats.xNumberOfSuccessfulAllocations, heapStats.xNumberOfSuccessfulFrees,
					heapStats.xNumberOfSuccessfulAllocations - heapStats.xNumberOfSuccessfulFrees);
				WSH_SHELL_PRINT("Free blocks num: %d\r\n\r\n", heapStats.xNumberOfFreeBlocks);

				WSH_SHELL_PRINT("Tasks in registry: %d\r\n", TasksRegistryMetrics.CurrNum);
				WSH_SHELL_PRINT("Tasks added to registry: %d\r\n", TasksRegistryMetrics.Added);
				WSH_SHELL_PRINT("Tasks removed from registry: %d\r\n\r\n",
								TasksRegistryMetrics.Removed);

				WSH_SHELL_PRINT(
					"# [Task num]. [Task name]: [Words stack high watermark] / [Bytes total] / [%% "
					"memory used]\r\n");

				u32 taskMaxNameLen = 0;
				for (u32 taskIdx = 0; taskIdx < NUM_ELEMENTS(TasksRegistry); taskIdx++) {
					if (!TasksRegistry[taskIdx].Handle)
						continue;

					u32 taskNameLen = strlen(pcTaskGetName(TasksRegistry[taskIdx].Handle));
					if (taskNameLen > taskMaxNameLen)
						taskMaxNameLen = taskNameLen;
				}

				taskMaxNameLen += 5;

				char taskNameStr[taskMaxNameLen];
				for (u32 taskIdx = 0; taskIdx < NUM_ELEMENTS(TasksRegistry); taskIdx++) {
					if (!TasksRegistry[taskIdx].Handle)
						continue;

					u32 n = snprintf(taskNameStr, NUM_ELEMENTS(taskNameStr), "'%s'",
									 pcTaskGetName(TasksRegistry[taskIdx].Handle));
					memset(&taskNameStr[n], '.', taskMaxNameLen - n);
					taskNameStr[taskMaxNameLen - 1] = '\0';

					u32 stackHwmBytes =
						uxTaskGetStackHighWaterMark(TasksRegistry[taskIdx].Handle) * sizeof(u32);
					float stackRatio = (float)stackHwmBytes /
									   ((float)TasksRegistry[taskIdx].StackInitBytes / 100.0f);
					stackRatio	 = 100.0f - stackRatio;
					char* pColor = stackRatio < 50	 ? ESC_COLOR_GREEN
								   : stackRatio < 75 ? ESC_COLOR_YELLOW
													 : ESC_COLOR_RED;

					WSH_SHELL_PRINT("#%3d. %s: %4d / %4d / %s%4.2f%%%s\r\n", taskIdx, taskNameStr,
									stackHwmBytes, TasksRegistry[taskIdx].StackInitBytes, pColor,
									stackRatio, ESC_COLOR_WHITE);
				}

				break;
			}

			case CMD_RTOS_OPT_INFO: {
				n +=
					sprintf(infoBuff + n, JSON_FIELD_STR_INT, "tasksNum", uxTaskGetNumberOfTasks());
				n += sprintf(infoBuff + n, JSON_FIELD_STR_INT, "availableHeapSpaceInBytes",
							 heapStats.xAvailableHeapSpaceInBytes);
				n += sprintf(infoBuff + n, JSON_FIELD_STR_INT, "everMinFreeRemainingInBytes",
							 heapStats.xMinimumEverFreeBytesRemaining);
				n += sprintf(infoBuff + n, JSON_FIELD_STR_INT, "largestFreeBlockInBytes",
							 heapStats.xSizeOfLargestFreeBlockInBytes);
				n += sprintf(infoBuff + n, JSON_FIELD_STR_INT, "smallestFreeBlockInBytes",
							 heapStats.xSizeOfSmallestFreeBlockInBytes);
				u32 ident = STRING_LIB_JSON_PRETTY_PRINT_DEF(infoBuff, prettyPrint,
															 RTOS_ANALYZER_SHELL_BUFF_SIZE);
				WSH_SHELL_PRINT(prettyPrint);

				infoBuff[0] = 0;
				n			= 0;
				n += sprintf(infoBuff + n, JSON_FIELD_STR_INT, "successfulAllocations",
							 heapStats.xNumberOfSuccessfulAllocations);
				n += sprintf(infoBuff + n, JSON_FIELD_STR_INT, "successfulFrees",
							 heapStats.xNumberOfSuccessfulFrees);
				n += sprintf(infoBuff + n, JSON_FIELD_STR_INT,
							 "freeBlocksNum:", heapStats.xNumberOfFreeBlocks);
				n += sprintf(infoBuff + n, JSON_FIELD_STR_INT,
							 "tasksInRegistry:", TasksRegistryMetrics.CurrNum);
				n += sprintf(infoBuff + n, JSON_FIELD_STR_INT,
							 "tasksAddedToRegistry:", TasksRegistryMetrics.Added);
				n += sprintf(infoBuff + n, JSON_FIELD_STR_INT,
							 "tasksRemovedFromRegistry:", TasksRegistryMetrics.Removed);
				ident = StringLib_JsonPrettyPrint(
					infoBuff, prettyPrint, RTOS_ANALYZER_SHELL_BUFF_SIZE, '\"', 4, "\r\n", ident);
				WSH_SHELL_PRINT(prettyPrint);

				infoBuff[0] = 0;
				n			= 0;
				n += sprintf(infoBuff + n, "\"tasksInfo\":[");

				u32 taskIdx = 0;
				for (; taskIdx < RTOS_ANALYZER_TASKS_MAX_NUM - 1; taskIdx++) {
					if (!TasksRegistry[taskIdx].Handle)
						continue;

					u32 stackHwmBytes =
						sizeof(u32) * uxTaskGetStackHighWaterMark(TasksRegistry[taskIdx].Handle);
					float stackRatio = (float)stackHwmBytes /
									   ((float)TasksRegistry[taskIdx].StackInitBytes / 100.0f);
					stackRatio = 100.0f - stackRatio;

					sprintf(infoBuff + n,
							"{\"name\":\"%s\",\"stack_hwm\":%d,\"stack_init\":%d,\"ratio\":%f}%s",
							pcTaskGetName(TasksRegistry[taskIdx].Handle), stackHwmBytes,
							TasksRegistry[taskIdx].StackInitBytes, stackRatio,
							((taskIdx + 1 < RTOS_ANALYZER_TASKS_MAX_NUM - 1) &&
							 TasksRegistry[taskIdx + 1].Handle)
								? ","
								: "],");
					ident = StringLib_JsonPrettyPrint(infoBuff, prettyPrint,
													  RTOS_ANALYZER_SHELL_BUFF_SIZE, '\"', 4,
													  "\r\n", ident);
					WSH_SHELL_PRINT(prettyPrint);

					infoBuff[0] = 0;
					n			= 0;
				}

				n += sprintf(infoBuff + n, JSON_FIELD_LAST, JSON_KEY_TSTAMP,
							 TimeDate_Timestamp_Get());
				ident = StringLib_JsonPrettyPrint(
					infoBuff, prettyPrint, RTOS_ANALYZER_SHELL_BUFF_SIZE, '\"', 4, "\r\n", ident);
				WSH_SHELL_PRINT(prettyPrint);
				break;
			}

			case CMD_RTOS_OPT_SUSPEND: {
				RET_STATE_t retState = RET_STATE_ERR_EMPTY;

				//TODO Check here if we can't stop the task

				SYS_CRITICAL_ON();
				for (u32 taskIdx = 0; taskIdx < NUM_ELEMENTS(TasksRegistry); taskIdx++) {
					if (!TasksRegistry[taskIdx].Handle)
						continue;

					//TODO check if task deleted

					if (TasksRegistry[taskIdx].Handle == ShellRoot_GetTaskHandle() ||
						TasksRegistry[taskIdx].Handle == WatchDog_GetTaskHandle() ||
						TasksRegistry[taskIdx].Handle == DebugSend_GetTaskHandle() ||
						TasksRegistry[taskIdx].Handle == TimerTask_Handle ||
						TasksRegistry[taskIdx].Handle == IDLE_Task_Handle) {
						WSH_SHELL_PRINT_WARN("* '%s' protected\r\n",
											 pcTaskGetName(TasksRegistry[taskIdx].Handle));
						continue;
					}

					if (eTaskGetState(TasksRegistry[taskIdx].Handle) == eDeleted ||
						eTaskGetState(TasksRegistry[taskIdx].Handle) == eSuspended ||
						eTaskGetState(TasksRegistry[taskIdx].Handle) == eInvalid) {
						WSH_SHELL_PRINT_WARN("* '%s' already has %d state\r\n",
											 pcTaskGetName(TasksRegistry[taskIdx].Handle),
											 eTaskGetState(TasksRegistry[taskIdx].Handle));
						continue;
					}

					vTaskSuspend(TasksRegistry[taskIdx].Handle);
					retState = RET_STATE_SUCCESS;

					TasksRegistry[taskIdx].RecoverAfterSuspend = true;
					WSH_SHELL_PRINT("* '%s' suspended\r\n",
									pcTaskGetName(TasksRegistry[taskIdx].Handle));
				}
				SYS_CRITICAL_OFF();

				WatchDog_TaskCreateOrProlongate(DELAY_1_MINUTE * 3);

				n += sprintf(infoBuff + n, JSON_FIELD_STR_STR, "act", optCtx.Option->LongName);
				n += sprintf(infoBuff + n, JSON_FIELD_STR_STR, "res", RetState_GetStr(retState));
				n += sprintf(infoBuff + n, JSON_FIELD_LAST, JSON_KEY_TSTAMP,
							 TimeDate_Timestamp_Get());

				STRING_LIB_JSON_PRETTY_PRINT_DEF(infoBuff, prettyPrint,
												 RTOS_ANALYZER_SHELL_BUFF_SIZE);
				WSH_SHELL_PRINT(prettyPrint);
				break;
			}

			case CMD_RTOS_OPT_RESUME: {
				RET_STATE_t retState = RET_STATE_ERR_EMPTY;

				SYS_CRITICAL_ON();
				for (u32 taskIdx = 0; taskIdx < NUM_ELEMENTS(TasksRegistry); taskIdx++) {
					if (!TasksRegistry[taskIdx].Handle)
						continue;

					if (TasksRegistry[taskIdx].RecoverAfterSuspend == true) {
						vTaskResume(TasksRegistry[taskIdx].Handle);
						retState = RET_STATE_SUCCESS;

						WSH_SHELL_PRINT("* '%s' resumed\r\n",
										pcTaskGetName(TasksRegistry[taskIdx].Handle));
						TasksRegistry[taskIdx].RecoverAfterSuspend = false;
					}
				}
				SYS_CRITICAL_OFF();

				n += sprintf(infoBuff + n, JSON_FIELD_STR_STR, "act", optCtx.Option->LongName);
				n += sprintf(infoBuff + n, JSON_FIELD_STR_STR, "res", RetState_GetStr(retState));
				n += sprintf(infoBuff + n, JSON_FIELD_LAST, JSON_KEY_TSTAMP,
							 TimeDate_Timestamp_Get());

				STRING_LIB_JSON_PRETTY_PRINT_DEF(infoBuff, prettyPrint,
												 RTOS_ANALYZER_SHELL_BUFF_SIZE);
				WSH_SHELL_PRINT(prettyPrint);
				break;
			}

			default:
				return WSH_SHELL_RET_STATE_ERROR;
		}
	}

	return WSH_SHELL_RET_STATE_SUCCESS;
}

const WshShellCmd_t Shell_RtosCmd = {
	.Groups	 = WSH_SHELL_CMD_GROUP_ADMIN,
	.Name	 = "rtos",
	.Descr	 = "RTOS interaction command",
	.Options = RtosOptArr,
	.OptNum	 = CMD_RTOS_OPT_ENUM_SIZE,
	.Handler = shell_cmd_rtos,
};

#else /* RTOS_ANALYZER */

void RTOS_Analyzer_ShellCmdInit(void) {
}

BaseType_t RTOS_Analyzer_CreateTask(TaskFunction_t taskCode, const char* const pName,
									configSTACK_DEPTH_TYPE stackDepth, void* pParameters,
									UBaseType_t priority, TaskHandle_t* pTaskHandle) {
	return xTaskCreate(taskCode, pName, stackDepth, pParameters, priority, pTaskHandle);
}

void RTOS_Analyzer_AddSystemTasksToRegistry(void) {
}

void RTOS_Analyzer_DeleteTask(TaskHandle_t* pTaskHandle) {
	TaskHandle_t tmpTaskHandle = *pTaskHandle;
	*pTaskHandle			   = NULL;
	vTaskDelete(tmpTaskHandle);
}

void RTOS_Analyzer_Check(void) {
}

#endif /* RTOS_ANALYZER */
