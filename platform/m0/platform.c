#include "platform.h"
#include "crc.h"
#include "gpio.h"
#include "int.h"
#include "platform_inc_m0.h"
#include "platform_int_cfg_m0.h"
#include "rng.h"
#include "rtc.h"
#include "sys.h"
#include "tim.h"
#include "mmc.h"
#include "usart.h"
#include "usb.h"

#if !defined(FW_PLATFORM_C0)
#error No platform definition
#endif /* !defined(FW_PLATFORM_C0) */

#if (NVIC_IRQ_PRIO_MAX >= 0x0F)
#error NVIC_IRQ_PRIO_MAX is too big
#endif /* (NVIC_IRQ_PRIO_MAX >= 0x0F) */

/**
 * 4 bits for pre-emption priority,
 * 0 bits for subpriority (for FreeRTOS)
 */
#define NVIC_PRIO_GROUP_0 ((u32)0x00000007)
#define NVIC_PRIO_GROUP_1 ((u32)0x00000006)
#define NVIC_PRIO_GROUP_2 ((u32)0x00000005)
#define NVIC_PRIO_GROUP_3 ((u32)0x00000004)
#define NVIC_PRIO_GROUP_4 ((u32)0x00000003)

void Pl_Stub_CommonClbk(void) {
}

void Pl_Stub_ParamClbk(void* pVal) {
}

void Pl_Stub_HardFaultClbk(u32 pcVal) {
	DISCARD_UNUSED(pcVal);
}

Pl_IsInit_t Pl_IsInit;
Pl_SysClock_t Pl_SysClk;

/**
 * @brief owerride HAL delay functions for usage inside 
 * HAL perif libs
 */
u32 HAL_GetTick(void) {
	return Pl_DelayMs_GetMsCnt();
}
void HAL_Delay(u32 delay) {
	u32 endTime = Pl_DelayMs_GetMsCnt() + delay;
	while (Pl_DelayMs_GetMsCnt() < endTime) {
	}
}

bool Pl_Init(Pl_HardFault_Clbk_t hardFault_Clbk, u32 maxMaskedIntPrio) {
	HardFault_SetCallback(hardFault_Clbk);

	extern u32 __start_no_cache_data[];
	LL_MPU_ConfigRegion(LL_MPU_REGION_NUMBER0, 0x0, (u32)__start_no_cache_data,
						LL_MPU_TEX_LEVEL0 | LL_MPU_REGION_SIZE_32KB | LL_MPU_REGION_FULL_ACCESS |
							LL_MPU_INSTRUCTION_ACCESS_ENABLE | LL_MPU_ACCESS_NOT_SHAREABLE |
							LL_MPU_ACCESS_NOT_CACHEABLE | LL_MPU_ACCESS_NOT_BUFFERABLE);

	extern u32 __start_no_init_data[];
	LL_MPU_ConfigRegion(LL_MPU_REGION_NUMBER1, 0x0, (u32)__start_no_init_data,
						LL_MPU_TEX_LEVEL0 | LL_MPU_REGION_SIZE_1KB | LL_MPU_REGION_FULL_ACCESS |
							LL_MPU_INSTRUCTION_ACCESS_ENABLE | LL_MPU_ACCESS_NOT_SHAREABLE |
							LL_MPU_ACCESS_NOT_CACHEABLE | LL_MPU_ACCESS_NOT_BUFFERABLE);

	LL_MPU_Enable(LL_MPU_CTRL_PRIVILEGED_DEFAULT);

	SCB_EnableICache();
	SCB_EnableDCache();

	LL_PWR_ConfigSupply(LL_PWR_LDO_SUPPLY);
	LL_PWR_SetRegulVoltageScaling(LL_PWR_REGU_VOLTAGE_SCALE0);
	while (!LL_PWR_IsActiveFlag_VOS()) {
	}

	LL_FLASH_SetLatency(LL_FLASH_LATENCY_3);
	while (LL_FLASH_GetLatency() != LL_FLASH_LATENCY_3) {
	}

	LL_SYSCFG_EnableCompensationCell();

	Sys_MainClock_Config();
	Sys_PerifClock_Config();
	Sys_RealTimeClock_Config();

	NVIC_SetPriorityGrouping(NVIC_PRIO_GROUP_4);

	// GPIO_MCO_Init(); //for pll tests

	LL_RCC_ClocksTypeDef RCC_Clocks;
	LL_RCC_GetSystemClocksFreq(&RCC_Clocks);
	Pl_SysClk.SYSCLK = RCC_Clocks.SYSCLK_Frequency;
	Pl_SysClk.HCLK	 = RCC_Clocks.HCLK_Frequency;
	Pl_SysClk.APB1	 = RCC_Clocks.PCLK1_Frequency;
	Pl_SysClk.APB2	 = RCC_Clocks.PCLK2_Frequency;
	Pl_SysClk.APB3	 = RCC_Clocks.PCLK3_Frequency;
	Pl_SysClk.APB4	 = RCC_Clocks.PCLK4_Frequency;

	Pl_IsInit.Hsi48Clk = true;
	Pl_IsInit.LsiClk   = LL_RCC_LSI_IsReady();
	Pl_IsInit.LseClk   = LL_RCC_LSE_IsReady();
	Pl_IsInit.Sys	   = true;
	return Pl_IsInit.Sys;
}

u32* Pl_UID_GetStrAndPtr(char* pDst) {
	return Sys_UID_GetStrAndPtr(pDst);
}

void Pl_CPU_GetStrAndPtr(char* pDst) {
	Sys_CPU_GetStrAndPtr(pDst);
}

u32 Pl_MCU_GetFlashSize(void) {
	return Sys_MCU_GetFlashSize();
}

void Pl_LedDebug_Init(void) {
	GPIO_LedDebug_Init();
}

void Pl_LedDebug_SetState(s32 state) {
	GPIO_ACTION_t act = state == 0 ? GPIO_RST : state == 1 ? GPIO_SET : GPIO_REV;
	GPIO_LedDebug_SetState(act);
}

bool Pl_DelayMs_Init(Pl_Common_Clbk_t pDelayTimerClbk) {
	Pl_IsInit.DelayMs = TIM_Delay_Init(pDelayTimerClbk);
	Sys_NVIC_SetPrioEnable(TIM_DELAY_IRQ, NVIC_IRQ_PRIO_TIM_DELAY);
	return Pl_IsInit.DelayMs;
}

void Pl_DelayMs_SuspendTimer(void) {
	TIM_Delay_Disable();
}

void Pl_DelayMs_ResumeTimer(void) {
	TIM_Delay_Enable();
}

u32 Pl_DelayMs_GetUsCnt(void) {
	return TIM_Delay_GetCnt();
}

u32 Pl_DelayMs_GetMsCnt(void) {
	return TIM_Delay_GetOvrflCnt();
}

bool Pl_Crc_Init(void) {
	Pl_IsInit.Crc32 = CRC_Init();
	return Pl_IsInit.Crc32;
}

void Pl_Crc_Reset(void) {
	CRC_Reset();
}

u32 Pl_Crc32_CheckBuff(const u8* pBuff, u32 buffSize) {
	ASSERT_CHECK(pBuff);
	return CRC_CheckBuff(pBuff, buffSize) * 0xFFFFFFFF - 1UL;
}

void Pl_SysCpuCnt_Init(void) {
	Pl_IsInit.CpuCounter = Sys_CounterCPU_Init();
}

u32 Pl_SysCpuCnt_Get(void) {
	return Sys_CounterCPU_Get();
}

void Pl_SoftReset(void) {
	Sys_MCU_Reset();
}

const char* Pl_GetRstFlagStr(void) {
	const char* pTmpStr = Sys_ResetFlag_GetStr();
	Sys_ResetFlag_Clear();

	return pTmpStr;
}

bool Pl_TrueRand_Init(void) {
	Pl_IsInit.TrueRand = RNG_Init();
	return Pl_IsInit.TrueRand;
}

bool Pl_TrueRand_GenerateBuff(u32* pBuff, u32 maxNum) {
	ASSERT_CHECK(pBuff);
	u32 tryCnt = 0;

	while (!RNG_GenerateBuff(pBuff, maxNum)) {
		if (++tryCnt > PL_TRNG_MAX_ITER) {
			PANIC();
			return false;
		}
	}

	return true;
}

u32 Pl_TrueRand_GenerateOne(void) {
	u32 rngVal = 0;
	u32 tryCnt = 0;

	while (!RNG_GenerateBuff(&rngVal, 1)) {
		if (++tryCnt > PL_TRNG_MAX_ITER) {
			PANIC();
			return 0;
		}
	}

	return rngVal;
}

bool Pl_DebugUart_Init(u32 baudRate, u8* pRxBuff, u32 rxBuffLen, Pl_UartExtra_RxClbk_t pRxClbk) {
	GPIO_UartDebug_Init();
	Pl_IsInit.SerialDebug = USART_Debug_Init(baudRate, pRxClbk);
	Sys_NVIC_SetPrioEnable(USART_DEBUG_IRQ, NVIC_IRQ_PRIO_USART_DEBUG);
	return Pl_IsInit.SerialDebug;
}

bool Pl_DebugUart_SendBuff(u8* pBuff, u32 size) {
	ASSERT_CHECK(pBuff);
	// TODO think about critical sections in platform SYS_CRITICAL_ON();
	bool txState = USART_Debug_TxData(pBuff, size, PL_UART_DEF_TMO);
	// SYS_CRITICAL_OFF();
	return txState;
}

u8 Pl_DebugUart_ReceiveByte(void) {
	return USART_Debug_RxByte();
}

bool Pl_RTC_Init(bool isInit, u32* pRetRtcStateBits) {
	Pl_IsInit.Rtc = isInit ? true : RTC_Init(pRetRtcStateBits);
	// Sys_NVIC_SetPrioEnable(RTC_WAKEUP_IRQ, NVIC_IRQ_PRIO_RTC_WAKEUP);
	return Pl_IsInit.Rtc;
}

bool Pl_RTC_IsReady(void) {
	return RTC_IsReady();
}

u32 Pl_RTC_GetYearOffset(void) {
	return RTC_YEAR_OFFSET;
}

bool Pl_RTC_SetDate(u16 year, u8 month, u8 day) {
	return RTC_Date_Set(year, month, day);
}

bool Pl_RTC_SetTime(u8 hour, u8 minute, u8 second) {
	return RTC_Time_Set(hour, minute, second);
}

void Pl_RTC_TimeDate_Get(u8* pHour, u8* pMinute, u8* pSecond, u8* pMonth, u8* pDay, u8* pWeekday,
						 u16* pYear) {
	ASSERT_CHECK(pHour);
	ASSERT_CHECK(pMinute);
	ASSERT_CHECK(pSecond);
	ASSERT_CHECK(pMonth);
	ASSERT_CHECK(pDay);
	ASSERT_CHECK(pWeekday);
	ASSERT_CHECK(pYear);

	*pHour	  = __LL_RTC_CONVERT_BCD2BIN(LL_RTC_TIME_GetHour(RTC));
	*pMinute  = __LL_RTC_CONVERT_BCD2BIN(LL_RTC_TIME_GetMinute(RTC));
	*pSecond  = __LL_RTC_CONVERT_BCD2BIN(LL_RTC_TIME_GetSecond(RTC));
	*pMonth	  = __LL_RTC_CONVERT_BCD2BIN(LL_RTC_DATE_GetMonth(RTC));
	*pDay	  = __LL_RTC_CONVERT_BCD2BIN(LL_RTC_DATE_GetDay(RTC));
	*pWeekday = LL_RTC_DATE_GetWeekDay(RTC);
	*pYear	  = __LL_RTC_CONVERT_BCD2BIN(LL_RTC_DATE_GetYear(RTC));
}

u32 Pl_BkpStorage_GetReg(u32 reg) {
	ASSERT_CHECK(reg <= LL_RTC_BKP_DR31);

	if (reg <= LL_RTC_BKP_DR31) {
		u32 regBkp = LL_RTC_BKP_DR0 + (u32)reg;
		return LL_RTC_BAK_GetRegister(RTC, regBkp);
	}

	return 0;
}

void Pl_BkpStorage_SetReg(u32 reg, u32 data) {
	ASSERT_CHECK(reg <= LL_RTC_BKP_DR31);

	if (reg <= LL_RTC_BKP_DR31) {
		u32 regBkp = LL_RTC_BKP_DR0 + (u32)reg;
		LL_RTC_BAK_SetRegister(RTC, regBkp, data);
	}
}

bool Pl_Watchdog_Init(void) {
	LL_IWDG_Enable(IWDG1);
	LL_IWDG_EnableWriteAccess(IWDG1);
	LL_IWDG_SetPrescaler(IWDG1, LL_IWDG_PRESCALER_256);
	LL_IWDG_SetReloadCounter(IWDG1, 0xFEE);
	while (!LL_IWDG_IsReady(IWDG1)) {
	}
	LL_IWDG_ReloadCounter(IWDG1);

	Pl_IsInit.Iwdg = true;
	return Pl_IsInit.Iwdg;
}

void Pl_Watchdog_ReloadCounter(void) {
	LL_IWDG_ReloadCounter(IWDG1);
}

void Pl_Sys_DebugInit(void) {
	LL_DBGMCU_EnableD1DebugInSleepMode();
	LL_DBGMCU_EnableD1DebugInStopMode();
	LL_DBGMCU_EnableD1DebugInStandbyMode();
	LL_DBGMCU_APB4_GRP1_FreezePeriph(LL_DBGMCU_APB4_GRP1_IWDG1_STOP);
}

bool Pl_USB_CDC_Init(Pl_Usb_RxClbk_t pRxClbk_USB, u8* pRxBuff) {
	Pl_IsInit.Usb = USB_Init(USBD_CLASS_CDC, pRxClbk_USB, pRxBuff);
	Sys_NVIC_SetPrioEnable(OTG_HS_IRQn, NVIC_IRQ_PRIO_USB_HS);
	return Pl_IsInit.Usb;
}

RET_STATE_t Pl_USB_CDC_Transmit(char* pBuff, u16 len) {
	return USB_SerialTransmit(pBuff, len);
}

bool Pl_USB_CDC_IsReady(void) {
	return USB_CDC_IsReady();
}

bool Pl_USB_IsClassCDC(void) {
	return USB_GetDeviceClass() == USBD_CLASS_CDC ? true : false;
}

bool Pl_USB_MSC_Init(void) {
	Pl_IsInit.Usb = USB_Init(USBD_CLASS_MSC, NULL, NULL);
	Sys_NVIC_SetPrioEnable(OTG_HS_IRQn, NVIC_IRQ_PRIO_USB_HS);
	return Pl_IsInit.Usb;
}

bool Pl_USB_IsClassMSC(void) {
	return USB_GetDeviceClass() == USBD_CLASS_MSC ? true : false;
}

bool Pl_USB_DeInit(void) {
	return USB_DeInit();
}


bool Pl_Emmc_Init(Pl_SdEmmcInfo_t* pSdEmmcInfo) {
	ASSERT_CHECK(pSdEmmcInfo != NULL);

	memset((void*)pSdEmmcInfo, 0, sizeof(Pl_SdEmmcInfo_t));

	GPIO_Emmc_Init();
	Pl_IsInit.SdEmmc = MMC_Emmc_Init(&pSdEmmcInfo->MfgID, pSdEmmcInfo->ProdName,
									 &pSdEmmcInfo->ProdRev, &pSdEmmcInfo->ProdSN);
	Sys_NVIC_SetPrioEnable(EMMC_IRQ, NVIC_IRQ_PRIO_EMMC);

	Pl_Emmc_GetDeviceInfo(pSdEmmcInfo);

	bool devInfoFlag =
		(bool)(pSdEmmcInfo->BlockNbr && pSdEmmcInfo->BlockSize && pSdEmmcInfo->CardType);
	// ASSERT_CHECK(devInfoFlag);

	return Pl_IsInit.SdEmmc && devInfoFlag;
}

bool Pl_Emmc_GetDeviceInfo(Pl_SdEmmcInfo_t* pSdEmmcInfo) {
	ASSERT_CHECK(pSdEmmcInfo != NULL);

	HAL_MMC_CardInfoTypeDef deviceInfo;
	memset((void*)&deviceInfo, 0, sizeof(HAL_MMC_CardInfoTypeDef));
	bool retState = MMC_Emmc_GetDeviceInfo(&deviceInfo);
	if (!retState) {
		PANIC();
		return false;
	}

	pSdEmmcInfo->CardType	  = deviceInfo.CardType;
	pSdEmmcInfo->Class		  = deviceInfo.Class;
	pSdEmmcInfo->RelCardAdd	  = deviceInfo.RelCardAdd;
	pSdEmmcInfo->BlockNbr	  = deviceInfo.BlockNbr;
	pSdEmmcInfo->BlockSize	  = deviceInfo.CardType;
	pSdEmmcInfo->LogBlockNbr  = deviceInfo.LogBlockNbr;
	pSdEmmcInfo->LogBlockSize = deviceInfo.LogBlockSize;

	return true;
}

bool Pl_Emmc_Read(u8* pData, u32 blockIdx, u32 blockNum) {
	return MMC_Emmc_ReadBlocks(pData, blockIdx, blockNum, PL_SD_EMMC_DEF_TMO);
}

bool Pl_Emmc_ReadDMA(u8* pData, u32 blockIdx, u32 blockNum) {
	return MMC_Emmc_ReadBlocksDMA(pData, blockIdx, blockNum);
}

bool Pl_Emmc_Write(const u8* pData, u32 blockIdx, u32 blockNum) {
	return MMC_Emmc_WriteBlocks(pData, blockIdx, blockNum, PL_SD_EMMC_DEF_TMO);
}

bool Pl_Emmc_WriteDMA(const u8* pData, u32 blockIdx, u32 blockNum) {
	return MMC_Emmc_WriteBlocksDMA(pData, blockIdx, blockNum);
}

bool Pl_Emmc_Erase(u32 startAddr, u32 endAddr) {
	return MMC_Emmc_Erase(startAddr, endAddr);
}

u32 Pl_Emmc_GetCardState(void) {
	return MMC_Emmc_GetCardState();
}

bool Pl_Emmc_IsCardInTransfer(void) {
	return MMC_Emmc_IsCardInTransfer();
}
