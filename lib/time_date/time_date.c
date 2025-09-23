#include "time_date.h"
#include "time_date_wrapper.h"

// Unix epoch time in Julian calendar (UnixTime = 00:00:00 01.01.1970 => JDN = 2440588)
#define JULIAN_DATE_BASE (2440588)

#define TD_OFFSET_DAY  (4)
#define TD_OFFSET_HOUR (12)
#define TD_OFFSET_MIN  (15)
#define TD_OFFSET_SEC  (18)
#define TD_OFFSET_MON  (0)
#define TD_OFFSET_YEAR (7)
#define TD_FORMAT_STR  "%04d-%02d-%02d %02d:%02d:%02d"

static s8 TimeDate_Zone = TD_ZONE_DEFAULT;
static TimeDateInterface_t TimeDateInterface;

//------------------------------------------------------------------------------

u32 TimeDate_TimeUnit_Init(void) {
	return TimeDateInterface.TimeUnitInit();
}

bool TimeDate_IsReady(void) {
	return TimeDateInterface.IsReady();
}

void TimeDate_TimeDate_Get(TimeDate_t* pTimeDate) {
	TimeDateInterface.TimeDate_Get(pTimeDate);
}

RET_STATE_t TimeDate_TimeDate_Set(TimeDate_t* pExtTimeDate) {
	if ((pExtTimeDate->Year < 1970U) || (pExtTimeDate->Year > 2099U) ||
		(pExtTimeDate->Month > 12U) || (pExtTimeDate->Month < 1U) || (pExtTimeDate->Hour >= 24U) ||
		(pExtTimeDate->Minute >= 60U) || (pExtTimeDate->Second >= 60U) ||
		(pExtTimeDate->Day > 31) || (pExtTimeDate->Day < 1U)) {
		return RET_STATE_ERROR;
	}

	return TimeDateInterface.TimeDate_Set(pExtTimeDate);
}

void TimeDate_Calendar_Check(TimeDate_t* pBuildTimeDate) {
	TimeDateInterface.CalendarCheck(pBuildTimeDate);
}

//------------------------------------------------------------------------------

u32 TimeDate_Init(void) {
	TimeDateWrapper_Init(&TimeDateInterface);
	return TimeDate_TimeUnit_Init();
}

void TimeDate_Struct_Init(TimeDate_t* pTimeDate) {
	memset(pTimeDate, 0, sizeof(TimeDate_t));
}

u32 TimeDate_TimeDateToTimestamp(TimeDate_t* pTimeDate) {
	// This hardcore math's are taken from http://en.wikipedia.org/wiki/Julian_day
	// Calculate some coefficients
	u8 a  = (14 - pTimeDate->Month) / 12;
	u16 y = pTimeDate->Year + 4800 - a;		  // years since 1 March, 4801 BC
	u8 m  = pTimeDate->Month + (12 * a) - 3;  // since 1 March, 4801 BC

	// Gregorian calendar date compute
	u32 JDN;
	JDN = pTimeDate->Day;
	JDN += (153 * m + 2) / 5;
	JDN += 365 * y;
	JDN += y / 4;
	JDN += -y / 100;
	JDN += y / 400;
	JDN = JDN - 32045;
	JDN = JDN - JULIAN_DATE_BASE;				 // Calculate from base date
	JDN *= TD_SECOND_IN_DAY;					 // Days to seconds
	JDN += pTimeDate->Hour * TD_SECOND_IN_HOUR;	 // ... and today seconds
	JDN += pTimeDate->Minute * TD_SECOND_IN_MINUTE;
	JDN += pTimeDate->Second;

	return JDN;
}

u32 TimeDate_Timestamp_Get(void) {
	TimeDate_t localTimeDate;
	TimeDate_Struct_Init(&localTimeDate);
	TimeDate_TimeDate_Get(&localTimeDate);
	return TimeDate_TimeDateToTimestamp(&localTimeDate);
}

void TimeDate_TimestampToTimeDate(u32 timestamp, TimeDate_t* pTimeDate) {
	u32 tm, t1, a, b, c, d, e, m;
	u64 JD	= ((timestamp + 43200) / (86400 >> 1)) + (2440587 << 1) + 1;
	u64 JDN = JD >> 1;

	tm				  = timestamp;
	t1				  = tm / 60;
	pTimeDate->Second = tm - (t1 * 60);
	tm				  = t1;
	t1				  = tm / 60;
	pTimeDate->Minute = tm - (t1 * 60);
	tm				  = t1;
	t1				  = tm / 24;
	pTimeDate->Hour	  = tm - (t1 * 24);

	a = JDN + 32044;
	b = ((4 * a) + 3) / 146097;
	c = a - ((146097 * b) / 4);
	d = ((4 * c) + 3) / 1461;
	e = c - ((1461 * d) / 4);
	m = ((5 * e) + 2) / 153;

	pTimeDate->WeekDay = JDN % 7;
	pTimeDate->Day	   = e - (((153 * m) + 2) / 5) + 1;
	pTimeDate->Month   = m + 3 - (12 * (m / 10));
	pTimeDate->Year	   = (100 * b) + d - 4800 + (m / 10);
	//STM32 RTC calculates WeekDay from 1 to 7, timestamp function from 0 to 6
	pTimeDate->WeekDay += 1;
}

//------------------------------------------------------------------------------

void TimeDate_BuildTimeDate_Get(TimeDate_t* pTimeDate) {
	//const char* weekStr = "Mon Tue Wed Thu Fri Sat Sun";
	const char* months = "Jan Feb Mar Apr May Jun Jul Aug Sep Oct Nov Dec";

	char buildTimeDateStr[TD_STR_SIZE];
	char tmpBuff[5];
	memset(buildTimeDateStr, '\0', NUM_ELEMENTS(buildTimeDateStr));
	sprintf(buildTimeDateStr, TD_BUILD_DATE_TIME);	//Jan 18 2022 22:25:03

	memset(tmpBuff, '\0',
		   NUM_ELEMENTS(tmpBuff));	//This functions order extremely significant!!
	pTimeDate->Day = (u8)strtoul((memcpy(tmpBuff, buildTimeDateStr + TD_OFFSET_DAY, 2)), NULL, 10);
	pTimeDate->Hour =
		(u8)strtoul((memcpy(tmpBuff, buildTimeDateStr + TD_OFFSET_HOUR, 2)), NULL, 10);
	pTimeDate->Minute =
		(u8)strtoul((memcpy(tmpBuff, buildTimeDateStr + TD_OFFSET_MIN, 2)), NULL, 10);
	pTimeDate->Second =
		(u8)strtoul((memcpy(tmpBuff, buildTimeDateStr + TD_OFFSET_SEC, 2)), NULL, 10);
	memcpy(tmpBuff, buildTimeDateStr + TD_OFFSET_MON, 3);
	pTimeDate->Month = (strstr(months, tmpBuff) - months) / 4 + 1;
	pTimeDate->Year =
		(u16)strtoul((memcpy(tmpBuff, buildTimeDateStr + TD_OFFSET_YEAR, 4)), NULL, 10);
	pTimeDate->Zone = TimeDate_Zone;
}

u32 TimeDate_BuildTimestamp_Get(void) {
	TimeDate_t localTimeDate;
	TimeDate_Struct_Init(&localTimeDate);
	TimeDate_BuildTimeDate_Get(&localTimeDate);
	return TimeDate_TimeDateToTimestamp(&localTimeDate);
}

u32 TimeDate_AllowedTimestamp_Get(void) {
	return TimeDate_BuildTimestamp_Get() - TD_SECOND_IN_DAY;
}

//------------------------------------------------------------------------------

void TimeDate_TimeZone_Set(s8 newTZ) {
	TimeDate_Zone = newTZ;
}

s8 TimeDate_TimeZone_Get(void) {
	return TimeDate_Zone;
}

void TimeDate_TimeZone_Adjust(s8 offset, TimeDate_t* pTimeDate) {
	u32 ts = TimeDate_TimeDateToTimestamp(pTimeDate) - offset * TD_SECOND_IN_HOUR;
	TimeDate_TimestampToTimeDate(ts, pTimeDate);
}

//------------------------------------------------------------------------------

RET_STATE_t TimeDate_TimeDateToStr(TimeDate_t* pTimeDate, char* str, u32 strLen) {
	if (strLen < TD_STR_SIZE) {
		PANIC();
		return RET_STATE_ERR_PARAM;
	}

	snprintf(str, strLen, TD_FORMAT_STR, pTimeDate->Year, pTimeDate->Month, pTimeDate->Day,
			 pTimeDate->Hour, pTimeDate->Minute, pTimeDate->Second);
	return RET_STATE_SUCCESS;
}

RET_STATE_t TimeDate_TimestampToTimeDateStr(u32 ts, char* str, u32 strLen) {
	TimeDate_t td;
	TimeDate_Struct_Init(&td);
	TimeDate_TimestampToTimeDate(ts, &td);
	return TimeDate_TimeDateToStr(&td, str, strLen);
}

RET_STATE_t TimeDate_TimeDateStr_Get(char* str, u32 strLen) {
	ASSERT_CHECK(strLen >= TD_STR_SIZE);

	TimeDate_t td;
	TimeDate_Struct_Init(&td);
	TimeDate_TimeDate_Get(&td);
	return TimeDate_TimeDateToStr(&td, str, strLen);
}

//------------------------------------------------------------------------------

void TimeDate_MsToTimeDate(TimeDate_t* pTimeDate, u32 ms) {
	u32 secondsCnt	  = ms / TD_MSECOND_IN_SECOND;
	pTimeDate->Second = secondsCnt % TD_SECOND_IN_MINUTE;
	pTimeDate->Minute = (secondsCnt / TD_SECOND_IN_MINUTE) % TD_MINUTE_IN_HOUR;
	pTimeDate->Hour	  = (secondsCnt / TD_SECOND_IN_HOUR) % TD_HOUR_IN_DAY;
	pTimeDate->Day	  = secondsCnt / TD_SECOND_IN_DAY;
}

bool TimeDate_IsCurrentTimeInPeriod(u32 enterHour, u32 exitHour, TimeDate_t* pTimeDate) {
	if (enterHour == exitHour)
		return true;

	u32 hour = pTimeDate->Hour;
	if (enterHour > exitHour)
		return (hour < enterHour && hour >= exitHour) ? false : true;
	else
		return (hour < exitHour && hour >= enterHour) ? true : false;
}

/**
 * @brief Compute difference between two u64 us timestamps
 * and cast in to one u32 ms
 * 
 * @param ts0 ts one in us
 * @param ts1 ts two in us
 * @return u32 delta ts in ms
 */
u32 TimeDate_DeltaUsToDeltaMs(u64 ts0, u64 ts1) {
	return (u32)(ts0 - ts1) / 1000;
}
