#ifndef __TIME_DATE_H
#define __TIME_DATE_H

#include "main.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Use timeout for RTC upadte and seconds updete
 */
#define TD_USE_RTC_UPDATE_TMO 0
#define RTC_UPD_TMO			  (10 * DELAY_1_MINUTE)
#define RTC_UPD_SEC_TMO		  (30)

#define TD_ZONE_DEFAULT (0)

#define TD_MSECOND_IN_SECOND (1000)
#define TD_SECOND_IN_MINUTE	 (60)
#define TD_MINUTE_IN_HOUR	 (60)
#define TD_HOUR_IN_DAY		 (24)
#define TD_SECOND_IN_HOUR	 (TD_SECOND_IN_MINUTE * TD_MINUTE_IN_HOUR)
#define TD_SECOND_IN_DAY	 (TD_SECOND_IN_HOUR * TD_HOUR_IN_DAY)
#define TD_MINUTE_IN_DAY	 (TD_MINUTE_IN_HOUR * TD_HOUR_IN_DAY)
#define TD_MSECOND_IN_DAY	 (TD_MSECOND_IN_SECOND * TD_SECOND_IN_DAY)
// #define TD_USEC2SEC(ts)			((ts) / 1000000.0f)

#define TD_STR_SIZE		   (32)
#define TD_BUILD_DATE_TIME (__DATE__ " " __TIME__)

typedef struct {
	u8 Second;
	u8 Minute;
	u8 Hour;
	s8 Zone;
	u8 Day;
	u8 WeekDay;
	u8 Month;
	u16 Year;
} TimeDate_t;

u32 TimeDate_TimeUnit_Init(void);
bool TimeDate_IsReady(void);
void TimeDate_TimeDate_Get(TimeDate_t* pTimeDate);
RET_STATE_t TimeDate_TimeDate_Set(TimeDate_t* pExtTimeDate);
void TimeDate_Calendar_Check(TimeDate_t* pBuildTimeDate);

u32 TimeDate_Init(void);
void TimeDate_Struct_Init(TimeDate_t* pTimeDate);
u32 TimeDate_Timestamp_Get(void);
u32 TimeDate_TimeDateToTimestamp(TimeDate_t* pTimeDate);
void TimeDate_TimestampToTimeDate(u32 timestamp, TimeDate_t* pTimeDate);

void TimeDate_BuildTimeDate_Get(TimeDate_t* pTimeDate);
u32 TimeDate_BuildTimestamp_Get(void);
u32 TimeDate_AllowedTimestamp_Get(void);

void TimeDate_TimeZone_Set(s8 newTZ);
s8 TimeDate_TimeZone_Get(void);
void TimeDate_TimeZone_Adjust(s8 offset, TimeDate_t* pTimeDate);

RET_STATE_t TimeDate_TimeDateToStr(TimeDate_t* pTimeDate, char* str, u32 strLen);
RET_STATE_t TimeDate_TimestampToTimeDateStr(u32 ts, char* str, u32 strLen);
RET_STATE_t TimeDate_TimeDateStr_Get(char* str, u32 strLen);

void TimeDate_MsToTimeDate(TimeDate_t* pTimeDate, u32 ms);
bool TimeDate_IsCurrentTimeInPeriod(u32 enterHour, u32 exitHour, TimeDate_t* pTimeDate);
u32 TimeDate_DeltaUsToDeltaMs(u64 ts0, u64 ts1);

#ifdef __cplusplus
}
#endif

#endif /* __TIME_DATE_H */
