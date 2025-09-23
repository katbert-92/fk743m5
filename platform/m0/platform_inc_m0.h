#ifndef __PLATFORM_INC_M0_H
#define __PLATFORM_INC_M0_H

#include "stm32h7xx.h"

#include "stm32h7xx_ll_adc.h"
#include "stm32h7xx_ll_bus.h"
#include "stm32h7xx_ll_cortex.h"
#include "stm32h7xx_ll_crc.h"
#include "stm32h7xx_ll_crs.h"
#include "stm32h7xx_ll_dac.h"
#include "stm32h7xx_ll_dma.h"
#include "stm32h7xx_ll_exti.h"
#include "stm32h7xx_ll_gpio.h"
#include "stm32h7xx_ll_i2c.h"
#include "stm32h7xx_ll_iwdg.h"
#include "stm32h7xx_ll_pwr.h"
#include "stm32h7xx_ll_rcc.h"
#include "stm32h7xx_ll_rng.h"
#include "stm32h7xx_ll_rtc.h"
#include "stm32h7xx_ll_sdmmc.h"
#include "stm32h7xx_ll_spi.h"
#include "stm32h7xx_ll_system.h"
#include "stm32h7xx_ll_tim.h"
#include "stm32h7xx_ll_usart.h"
#include "stm32h7xx_ll_utils.h"
#include "stm32h7xx_ll_wwdg.h"

#include "stm32h7xx_hal.h"

#if !defined PLATFORM_M0_CLOCK_RTC_LSE && !defined PLATFORM_M0_CLOCK_RTC_LSI
#define PLATFORM_M0_CLOCK_RTC_LSE
#endif /* !defined PLATFORM_M0_CLOCK_RTC_LSE && !defined PLATFORM_M0_CLOCK_RTC_LSI */

// TODO it depends on cpu freq
#define RTC_LSI_CYCLES_TMO	10000
#define RTC_LSE_CYCLES_TMO	10000000
#define RTC_LSE_MAX_ATTEMPS 3

#define RTC_YEAR_OFFSET 2000

// TODO check values
#define RTC_ASYNCH_PREDIV	 127  //0x7F
#define RTC_SYNCH_PREDIV_LSE 255  //0xFF
#define RTC_SYNCH_PREDIV_LSI 249  //0xF9


#endif /* __PLATFORM_INC_M0_H */
