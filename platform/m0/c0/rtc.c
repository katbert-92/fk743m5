#include "rtc.h"
#include "sys.h"

bool RTC_Init(u32* pRetRtcStateBits) {
	if (!Pl_IsInit.Sys || !Pl_IsInit.LsiClk || !Pl_IsInit.LseClk) {
		PANIC();
		return false;
	}

	LL_RTC_InitTypeDef RTC_InitStruct;
	LL_RTC_StructInit(&RTC_InitStruct);

	if (pRetRtcStateBits) {
		*pRetRtcStateBits = 0;	//combine potential errors to event.value variable
		*pRetRtcStateBits |= LL_RCC_LSI_IsReady();
		*pRetRtcStateBits |= LL_RCC_LSE_IsReady() << 1;
		*pRetRtcStateBits |= LL_RCC_GetRTCClockSource() >> 6;
	}

	LL_RCC_EnableRTC();

	RTC_InitStruct.HourFormat	   = LL_RTC_HOURFORMAT_24HOUR;
	RTC_InitStruct.AsynchPrescaler = RTC_ASYNCH_PREDIV;
	if (Sys_LSE_IsReadyAndClkSrc())
		RTC_InitStruct.SynchPrescaler = RTC_SYNCH_PREDIV_LSE;
	else if (Sys_LSI_IsReadyAndClkSrc())
		RTC_InitStruct.SynchPrescaler = RTC_SYNCH_PREDIV_LSI;
	else
		PANIC();

	if (LL_RTC_Init(RTC, &RTC_InitStruct) != SUCCESS) {
		PANIC();
		return false;
	}

	if (pRetRtcStateBits)
		*pRetRtcStateBits |= LL_RCC_IsEnabledRTC() << 4;

	return true;
}

bool RTC_IsReady(void) {
	return (bool)(Sys_LSE_IsReadyAndClkSrc() || Sys_LSI_IsReadyAndClkSrc());
}

bool RTC_Time_Set(u8 hour, u8 minute, u8 second) {
	LL_RTC_TimeTypeDef RTC_TimeStruct;
	LL_RTC_TIME_StructInit(&RTC_TimeStruct);

	RTC_TimeStruct.Hours	  = hour;
	RTC_TimeStruct.Minutes	  = minute;
	RTC_TimeStruct.Seconds	  = second;
	RTC_TimeStruct.TimeFormat = LL_RTC_TIME_FORMAT_AM_OR_24;
	if (LL_RTC_TIME_Init(RTC, LL_RTC_FORMAT_BIN, &RTC_TimeStruct) == SUCCESS)
		return true;

	return false;
}

bool RTC_Date_Set(u16 year, u8 month, u8 day) {
	LL_RTC_DateTypeDef RTC_DateStruct;
	LL_RTC_DATE_StructInit(&RTC_DateStruct);

	RTC_DateStruct.Day	 = day;
	RTC_DateStruct.Month = month;
	//RTC_DateStruct.WeekDay = pNewDate->WeekDay;
	RTC_DateStruct.Year	 = year - RTC_YEAR_OFFSET;
	if (LL_RTC_DATE_Init(RTC, LL_RTC_FORMAT_BIN, &RTC_DateStruct) == SUCCESS)
		return true;

	return false;
}

// void RTC_WakeUpTim_InitAndSet(u16 sec)
// {
// 	EXTI_WakeUp_Init();
// 	Sys_NVIC_SetPrioEnable(RTC_WKUP_IRQn, NVIC_IRQ_PRIO_RTC_WAKEUP);

// 	LL_RTC_DisableWriteProtection(RTC);

// 	LL_RTC_WAKEUP_Disable(RTC);
// 	while(!LL_RTC_IsActiveFlag_WUTW(RTC));

// 	LL_RTC_WAKEUP_SetAutoReload(RTC, sec);
// 	LL_RTC_WAKEUP_SetClock(RTC, LL_RTC_WAKEUPCLOCK_CKSPRE);

// 	LL_RTC_EnableIT_WUT(RTC);
// 	LL_RTC_WAKEUP_Enable(RTC);

// 	LL_RTC_EnableWriteProtection(RTC);

// 	LL_RTC_ClearFlag_WUT(RTC);

// 	EXTI_WakeUp_Clear();
// }

// void RTC_WKUP_IRQHandler(void)
// {
// 	if(LL_RTC_IsActiveFlag_WUT(RTC))
// 	{
// 		EXTI_WakeUp_Clear();
// 		LL_RTC_ClearFlag_WUT(RTC);
// 	}
// }
