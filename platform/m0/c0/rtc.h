#ifndef __RTC_H
#define __RTC_H

#include "main.h"
#include "platform.h"
#include "platform_inc_m0.h"

#define RTC_WAKEUP_IRQ RTC_WKUP_IRQn

bool RTC_Init(u32* pRetRtcStateBits);
bool RTC_IsReady(void);
bool RTC_Time_Set(u8 hour, u8 minute, u8 second);
bool RTC_Date_Set(u16 year, u8 month, u8 day);

#endif /*__RTC_H */
