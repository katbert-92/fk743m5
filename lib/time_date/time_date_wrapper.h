#ifndef __TIME_DATE_WRAPPER_H
#define __TIME_DATE_WRAPPER_H

#include "time_date.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef u32 (*TimeDate_TimeUnit_Init_t)(void);
typedef bool (*TimeDate_IsReady_t)(void);
typedef void (*TimeDate_TimeDate_Get_t)(TimeDate_t* pTimeDate);
typedef RET_STATE_t (*TimeDate_TimeDate_Set_t)(TimeDate_t* pTimeDate);
typedef void (*TimeDate_Calendar_Check_t)(TimeDate_t* buildTimeDate);

typedef struct {
	TimeDate_TimeUnit_Init_t TimeUnitInit;
	TimeDate_IsReady_t IsReady;
	TimeDate_TimeDate_Get_t TimeDate_Get;
	TimeDate_TimeDate_Set_t TimeDate_Set;
	TimeDate_Calendar_Check_t CalendarCheck;
} TimeDateInterface_t;

void TimeDateWrapper_Init(TimeDateInterface_t* pTimeDateInterface);

#ifdef __cplusplus
}
#endif

#endif /* __TIME_DATE_WRAPPER_H */
