#include "time_date_wrapper.h"
#include "bkp_storage.h"
#include "platform.h"

#if TD_USE_RTC_UPDATE_TMO
#include "delay.h"
#endif /* TD_USE_RTC_UPDATE_TMO */

#define RTC_ENABLE_MARKER (0x2fb9c3a7)
#define RTC_CALEND_MARKER (0xc3012ca2)

static bool TimeDateWrapper_IsReady(void);
static void TimeDateWrapper_TimeDate_Get(TimeDate_t* pTimeDate);
static RET_STATE_t TimeDateWrapper_TimeDate_Set(TimeDate_t* pTimeDate);
static u32 TimeDateWrapper_TimeUnit_Init(void);
static void TimeDateWrapper_Calendar_Check(TimeDate_t* pBuildTimeDate);

void TimeDateWrapper_Init(TimeDateInterface_t* pTimeDateInterface) {
	pTimeDateInterface->IsReady		  = TimeDateWrapper_IsReady;
	pTimeDateInterface->TimeUnitInit  = TimeDateWrapper_TimeUnit_Init;
	pTimeDateInterface->TimeDate_Get  = TimeDateWrapper_TimeDate_Get;
	pTimeDateInterface->TimeDate_Set  = TimeDateWrapper_TimeDate_Set;
	pTimeDateInterface->CalendarCheck = TimeDateWrapper_Calendar_Check;
}

static u32 TimeDateWrapper_TimeUnit_Init(void) {
	u32 rtcStateBits		 = 0;
	RET_STATE_t rtcInitState = RET_STATE_UNDEF;
	bool rtcIsInit = BkpStorage_GetValue(BKP_KEY_RTC_STATE) == RTC_ENABLE_MARKER ? true : false;
	rtcInitState   = Pl_RTC_Init(rtcIsInit, &rtcStateBits);

	if (!rtcIsInit && rtcInitState == RET_STATE_SUCCESS)
		BkpStorage_SetValue(BKP_KEY_RTC_STATE, RTC_ENABLE_MARKER);

	if (BkpStorage_GetValue(BKP_KEY_RTC_STATE) == RTC_ENABLE_MARKER)
		rtcStateBits |= 0x01 << 5;

	return rtcStateBits;
}

static bool TimeDateWrapper_IsReady(void) {
	bool RTC_MarkerIsCreated = BkpStorage_GetValue(BKP_KEY_RTC_STATE) == RTC_ENABLE_MARKER;
	return RTC_MarkerIsCreated && Pl_RTC_IsReady();
}

static void TimeDateWrapper_TimeDate_Get(TimeDate_t* pTimeDate) {
	Pl_RTC_TimeDate_Get(&(pTimeDate->Hour), &(pTimeDate->Minute), &(pTimeDate->Second),
						&(pTimeDate->Month), &(pTimeDate->Day), &(pTimeDate->WeekDay),
						&(pTimeDate->Year));

	pTimeDate->Year += Pl_RTC_GetYearOffset();
}

static RET_STATE_t TimeDateWrapper_TimeDate_Set(TimeDate_t* pExtTimeDate) {
	u32 newTS = TimeDate_TimeDateToTimestamp(pExtTimeDate);
	if (newTS < TimeDate_AllowedTimestamp_Get())
		return RET_STATE_ERR_PARAM;

	u32 currTS = TimeDate_Timestamp_Get();

	if (!Pl_RTC_IsReady())
		return RET_STATE_ERROR;

#if TD_USE_RTC_UPDATE_TMO
	static s32 lastUpdate = 0 - RTC_UPD_DELAY;
	if (Delay_TimeMilliSec_Get() < lastUpdate + RTC_UPD_DELAY)
		return RET_STATE_ERR_BUSY;
	lastUpdate = xTaskGetTickCount();
#endif /* TD_USE_RTC_UPDATE_TMO */

	bool isChanged = false;

	TimeDate_t locTd;
	TimeDate_Struct_Init(&locTd);
	TimeDateWrapper_TimeDate_Get(&locTd);
	if (pExtTimeDate->Year != locTd.Year || pExtTimeDate->Month != locTd.Month ||
		pExtTimeDate->Day != locTd.Day) {
		Pl_RTC_SetDate(pExtTimeDate->Year, pExtTimeDate->Month, pExtTimeDate->Day);
		isChanged = true;
	}

	u32 deltaTs = currTS > newTS ? currTS - newTS : newTS - currTS;
	if (deltaTs > RTC_UPD_SEC_TMO) {
		Pl_RTC_SetTime(pExtTimeDate->Hour, pExtTimeDate->Minute, pExtTimeDate->Second);
		isChanged = true;
	}

	return isChanged == true ? RET_STATE_SUCCESS : RET_STATE_ERR_EMPTY;
}

static void TimeDateWrapper_Calendar_Check(TimeDate_t* pBuildTimeDate) {
	if (BkpStorage_GetValue(BKP_KEY_CALEND_MARKER) != RTC_CALEND_MARKER) {
		Pl_RTC_SetDate(pBuildTimeDate->Year, pBuildTimeDate->Month, pBuildTimeDate->Day);
		Pl_RTC_SetTime(pBuildTimeDate->Hour, pBuildTimeDate->Minute, pBuildTimeDate->Second);
		BkpStorage_SetValue(BKP_KEY_CALEND_MARKER, RTC_CALEND_MARKER);
	}
}
