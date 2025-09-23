#ifndef __RTOS_ANALYZER_H
#define __RTOS_ANALYZER_H

#include "def_rtos.h"
#include "main.h"

void RTOS_Analyzer_ShellCmdInit(void);
BaseType_t RTOS_Analyzer_CreateTask(TaskFunction_t taskCode, const char* const pName,
									configSTACK_DEPTH_TYPE stackDepth, void* pParameters,
									UBaseType_t priority, TaskHandle_t* pTaskHandle);
void RTOS_Analyzer_AddSystemTasksToRegistry(void);
void RTOS_Analyzer_DeleteTask(TaskHandle_t* pTaskHandle);
void RTOS_Analyzer_Check(void);

#endif /* __RTOS_ANALYZER_H */
