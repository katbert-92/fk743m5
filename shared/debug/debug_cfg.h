#ifndef __DEBUG_CFG_H
#define __DEBUG_CFG_H

#define DBG_USE_TIME_DATE 1
#define DBG_USE_SYS_TIMER 1
#define DBG_USE_RTOS	  1
#define DBG_USE_FILE_NAME 1

#if DBG_USE_TIME_DATE
#include "time_date.h"
#define DBG_TIME_DATE_STR_GET(a, b) TimeDate_TimeDateStr_Get(a, b)
#else /* DBG_USE_TIME_DATE */
#define TD_STR_SIZE					(8)
#define DBG_TIME_DATE_STR_GET(a, b) snprintf(a, b, "n/a")
#endif /* DBG_USE_TIME_DATE */

#if DBG_USE_SYS_TIMER
#include "delay.h"
#define DBG_TIMER_MILLISEC_GET() Delay_TimeMilliSec_Get()
#define DBG_TIMER_MICROSEC_GET() Delay_TimeMicroSec_Get()
#else /* DBG_USE_SYS_TIMER */
#define DBG_TIMER_MILLISEC_GET() 0
#define DBG_TIMER_MICROSEC_GET() 0
#endif /* DBG_USE_SYS_TIMER */

#if DBG_USE_RTOS
#define TASK_NAME_STR_SIZE 32
#define DBG_TASK_NAME_STR_GET(a, b)                                                       \
	do {                                                                                  \
		TaskHandle_t currTaskHdl = xTaskGetCurrentTaskHandle();                           \
		snprintf(a, b, currTaskHdl ? pcTaskGetName(xTaskGetCurrentTaskHandle()) : "n/a"); \
	} while (0)

#else /* DnjkjBG_USE_RTOS */
#define TASK_NAME_STR_SIZE			4
#define DBG_TASK_NAME_STR_GET(a, b) snprintf(a, b, "n/a")
#endif /* DBG_USE_RTOS */

#if DBG_USE_FILE_NAME
#include "stringlib.h"
#define DBG_FILENAME __FILENAME__
#else /* DBG_USE_FILE_NAME */
#define DBG_FILENAME __FILE__
#endif /* DBG_USE_FILE_NAME */

#endif /* __DEBUG_CFG_H */
