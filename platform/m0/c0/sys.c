#include "sys.h"

#define ARM_CM_DEMCR	  (*(u32*)0xE000EDFC)
#define ARM_CM_DWT_CTRL	  (*(u32*)0xE0001000)
#define ARM_CM_DWT_CYCCNT (*(u32*)0xE0001004)

#define X_ENTRY(flag, flag_str) flag_str,
static const char* ResetSrcList[] = {SYS_RST_SRC_TABLE()};
#undef X_ENTRY

const char* Sys_ResetFlag_GetStr(void) {
	return ResetSrcList[Sys_ResetFlag_Get()];
}

SYS_RESET_FLAG_t Sys_ResetFlag_Get(void) {
	SYS_RESET_FLAG_t rstFlag = SYS_RESET_FLAG_UNKNOWN;

	if (LL_RCC_IsActiveFlag_WWDG1RST())
		rstFlag = SYS_RESET_FLAG_WWDG;
	else if (LL_RCC_IsActiveFlag_IWDG1RST())
		rstFlag = SYS_RESET_FLAG_IWDG;
	else if (LL_RCC_IsActiveFlag_SFTRST())
		rstFlag = SYS_RESET_FLAG_SOFT;
	else if (LL_RCC_IsActiveFlag_PORRST())
		rstFlag = SYS_RESET_FLAG_POR;
	else if (LL_RCC_IsActiveFlag_PINRST())
		rstFlag = SYS_RESET_FLAG_PIN;
	else if (LL_RCC_IsActiveFlag_BORRST())
		rstFlag = SYS_RESET_FLAG_BOR;
	else if (LL_RCC_IsActiveFlag_D1RST())
		rstFlag = SYS_RESET_FLAG_D1;
	else if (LL_RCC_IsActiveFlag_D2RST())
		rstFlag = SYS_RESET_FLAG_D2;
	else if (LL_RCC_IsActiveFlag_CPURST())
		rstFlag = SYS_RESET_FLAG_CPU;

	return rstFlag;
}

void Sys_ResetFlag_Clear(void) {
	LL_RCC_ClearResetFlags();
}

static bool Sys_EnableLSI(void) {
	bool lsiClk = false;

	if (!LL_RCC_LSI_IsReady()) {
		LL_RCC_ForceBackupDomainReset();
		LL_RCC_ReleaseBackupDomainReset();
		LL_RCC_LSI_Enable();

		volatile u32 lsiWaitCnt = 0;
		while (!LL_RCC_LSI_IsReady() && lsiWaitCnt < RTC_LSI_CYCLES_TMO)
			lsiWaitCnt++;

		if (lsiWaitCnt < RTC_LSI_CYCLES_TMO)
			lsiClk = true;
	} else
		lsiClk = true;

	ASSERT_CHECK(LL_RCC_LSI_IsReady());
	return lsiClk;
}

static bool Sys_EnableLSE(void) {
	bool lseClk = false;

	if (!LL_RCC_LSE_IsReady()) {
		LL_RCC_ForceBackupDomainReset();
		LL_RCC_ReleaseBackupDomainReset();
		LL_RCC_LSE_Disable();
		LL_RCC_LSE_Enable();

		volatile u32 lseWaitCnt = 0;
		while (!LL_RCC_LSE_IsReady() && lseWaitCnt < RTC_LSE_CYCLES_TMO)
			lseWaitCnt++;

		if (lseWaitCnt < RTC_LSE_CYCLES_TMO)
			lseClk = true;
	} else
		lseClk = true;

	ASSERT_CHECK(LL_RCC_LSE_IsReady());
	return lseClk;
}

__INLINE bool Sys_LSI_IsReadyAndClkSrc(void) {
	return (bool)(LL_RCC_GetRTCClockSource() == LL_RCC_RTC_CLKSOURCE_LSI && LL_RCC_LSI_IsReady());
}

__INLINE bool Sys_LSE_IsReadyAndClkSrc(void) {
	return (bool)(LL_RCC_GetRTCClockSource() == LL_RCC_RTC_CLKSOURCE_LSE && LL_RCC_LSE_IsReady());
}

bool Sys_RealTimeClock_Config(void) {
	if (!LL_PWR_IsEnabledBkUpAccess())
		LL_PWR_EnableBkUpAccess();

#ifdef PLATFORM_M0_CLOCK_RTC_LSI
	if (Sys_LSI_IsReadyAndClkSrc())
		return true;

	if (Sys_EnableLSI())
		LL_RCC_SetRTCClockSource(LL_RCC_RTC_CLKSOURCE_LSI);

	if (!Sys_LSI_IsReadyAndClkSrc()) {
		PANIC();
		return false;
	}

	ASSERT_CHECK(LL_RCC_GetRTCClockSource() == LL_RCC_RTC_CLKSOURCE_LSI);
#endif /* PLATFORM_M0_CLOCK_RTC_LSI */

#ifdef PLATFORM_M0_CLOCK_RTC_LSE
	for (u32 attempt = 0; attempt < RTC_LSE_MAX_ATTEMPS; attempt++) {
		if (Sys_LSE_IsReadyAndClkSrc())
			break;

		if (Sys_EnableLSE()) {
			LL_RCC_SetRTCClockSource(LL_RCC_RTC_CLKSOURCE_LSE);
		}
	}

	if (!Sys_LSE_IsReadyAndClkSrc()) {
		PANIC();
		return false;
	}

	ASSERT_CHECK(LL_RCC_GetRTCClockSource() == LL_RCC_RTC_CLKSOURCE_LSE);
#endif /* PLATFORM_M0_CLOCK_RTC_LSE */

	return true;
}

void Sys_MainClock_Config(void) {
	LL_RCC_HSE_Enable();
	while (!LL_RCC_HSE_IsReady()) {
	}

	LL_RCC_HSI_Enable();

	/* Wait till HSI is ready */
	while (!LL_RCC_HSI_IsReady()) {
	}
	LL_RCC_HSI_SetCalibTrimming(64);
	LL_RCC_HSI_SetDivider(LL_RCC_HSI_DIV1);
	LL_RCC_HSI48_Enable();

	while (!LL_RCC_HSI48_IsReady()) {
	}

	LL_RCC_PLL_SetSource(LL_RCC_PLLSOURCE_HSE);

	LL_RCC_PLL1_SetVCOInputRange(LL_RCC_PLLINPUTRANGE_1_2);
	LL_RCC_PLL1_SetVCOOutputRange(LL_RCC_PLLVCORANGE_MEDIUM);
	LL_RCC_PLL1_SetM(HSE_VALUE / FREQ_1_MHZ);
	LL_RCC_PLL1_SetN(420);
	LL_RCC_PLL1_SetP(1);
	LL_RCC_PLL1_SetQ(4);
	LL_RCC_PLL1_SetR(4);
	LL_RCC_PLL1P_Enable();
	LL_RCC_PLL1Q_Enable();
	LL_RCC_PLL1R_Enable();
	LL_RCC_PLL1FRACN_Disable();
	LL_RCC_PLL1_Enable();
	while (!LL_RCC_PLL1_IsReady()) {
	}

	LL_RCC_PLL2_SetVCOInputRange(LL_RCC_PLLINPUTRANGE_1_2);
	LL_RCC_PLL2_SetVCOOutputRange(LL_RCC_PLLVCORANGE_MEDIUM);
	LL_RCC_PLL2_SetM(HSE_VALUE / FREQ_1_MHZ);
	LL_RCC_PLL2_SetN(200);
	LL_RCC_PLL2_SetP(2);
	LL_RCC_PLL2_SetQ(2);
	LL_RCC_PLL2_SetR(2);
	LL_RCC_PLL2R_Enable();
	LL_RCC_PLL2FRACN_Disable();
	LL_RCC_PLL2_Enable();
	while (!LL_RCC_PLL2_IsReady()) {
	}

	LL_RCC_PLL3_SetVCOInputRange(LL_RCC_PLLINPUTRANGE_1_2);
	LL_RCC_PLL3_SetVCOOutputRange(LL_RCC_PLLVCORANGE_MEDIUM);
	LL_RCC_PLL3_SetM(HSE_VALUE / FREQ_1_MHZ);
	LL_RCC_PLL3_SetN(192);
	LL_RCC_PLL3_SetP(4);
	LL_RCC_PLL3_SetQ(4);
	LL_RCC_PLL3_SetR(4);
	LL_RCC_PLL3R_Enable();
	LL_RCC_PLL3FRACN_Disable();
	LL_RCC_PLL3_Enable();
	while (!LL_RCC_PLL3_IsReady()) {
	}

	//RM Figure 49. Core and bus clock generation, p. 332
	LL_RCC_SetSysPrescaler(LL_RCC_SYSCLK_DIV_1);  //D1CPRE
	LL_RCC_SetAHBPrescaler(LL_RCC_AHB_DIV_2);	  //HPRE
	LL_RCC_SetAPB1Prescaler(LL_RCC_APB1_DIV_2);	  //D2PPRE1
	LL_RCC_SetAPB2Prescaler(LL_RCC_APB2_DIV_2);	  //D2PPRE2
	LL_RCC_SetAPB3Prescaler(LL_RCC_APB3_DIV_2);	  //D1PPRE
	LL_RCC_SetAPB4Prescaler(LL_RCC_APB4_DIV_2);	  //D3PPRE

	LL_RCC_SetSysClkSource(LL_RCC_SYS_CLKSOURCE_PLL1);
	while (LL_RCC_GetSysClkSource() != LL_RCC_SYS_CLKSOURCE_STATUS_PLL1) {
	}

	LL_RCC_HSI48_Enable();
	while (!LL_RCC_HSI48_IsReady()) {
	}

	//For IWDG or RTC
	LL_PWR_EnableBkUpAccess();
	Sys_EnableLSI();

	SystemCoreClockUpdate();
}

void Sys_PerifClock_Config(void) {
	LL_APB4_GRP1_EnableClock(LL_APB4_GRP1_PERIPH_SYSCFG);

	LL_AHB4_GRP1_EnableClock(LL_AHB4_GRP1_PERIPH_GPIOA);
	LL_AHB4_GRP1_EnableClock(LL_AHB4_GRP1_PERIPH_GPIOB);
	LL_AHB4_GRP1_EnableClock(LL_AHB4_GRP1_PERIPH_GPIOC);
	LL_AHB4_GRP1_EnableClock(LL_AHB4_GRP1_PERIPH_GPIOD);
	LL_AHB4_GRP1_EnableClock(LL_AHB4_GRP1_PERIPH_GPIOE);
	LL_AHB4_GRP1_EnableClock(LL_AHB4_GRP1_PERIPH_GPIOF);
	LL_AHB4_GRP1_EnableClock(LL_AHB4_GRP1_PERIPH_GPIOG);
	LL_AHB1_GRP1_EnableClock(LL_AHB4_GRP1_PERIPH_GPIOH);

	LL_AHB1_GRP1_EnableClock(LL_AHB1_GRP1_PERIPH_DMA1);
	// LL_AHB1_GRP1_EnableClock(LL_AHB1_GRP1_PERIPH_DMA2);

	LL_RCC_SetRNGClockSource(LL_RCC_RNG_CLKSOURCE_HSI48);

	LL_RCC_SetUSARTClockSource(LL_RCC_USART234578_CLKSOURCE_PCLK1);
	LL_RCC_SetUSARTClockSource(LL_RCC_USART16_CLKSOURCE_PCLK2);

	LL_RCC_SetI2CClockSource(LL_RCC_I2C123_CLKSOURCE_PCLK1);
	LL_RCC_SetI2CClockSource(LL_RCC_I2C4_CLKSOURCE_PCLK4);

	LL_RCC_SetSPIClockSource(LL_RCC_SPI123_CLKSOURCE_PLL1Q);

	LL_PWR_EnableUSBReg();
	LL_PWR_EnableUSBVoltageDetector();
	LL_RCC_SetUSBClockSource(LL_RCC_USB_CLKSOURCE_PLL3Q);
	LL_AHB1_GRP1_EnableClock(LL_AHB1_GRP1_PERIPH_USB1OTGHS);

	LL_RCC_SetSDMMCClockSource(LL_RCC_SDMMC_CLKSOURCE_PLL2R);  //LL_RCC_SDMMC_CLKSOURCE_PLL1Q
	// LL_AHB3_GRP1_EnableClock(LL_AHB3_GRP1_PERIPH_SDMMC1); //enabled in platform

	// LL_RCC_SetOSPIClockSource(LL_RCC_OSPI_CLKSOURCE_PLL2R);
	// LL_AHB3_GRP1_EnableClock(LL_AHB3_GRP1_PERIPH_OCTOSPIM); //enabled in platform
	// LL_AHB3_GRP1_EnableClock(LL_AHB3_GRP1_PERIPH_OSPI1); //enabled in platform
}

u32* Sys_UID_GetStrAndPtr(char* pDst) {
	u32* UIDptr = (u32*)UID_BASE;
	if (pDst)
		sprintf(pDst, "%08x-%08x-%08x", UIDptr[2], UIDptr[1], UIDptr[0]);

	return UIDptr;
}

void Sys_CPU_GetStrAndPtr(char* pDst) {
	if (pDst)
		sprintf(pDst, "%04x/%04x", LL_DBGMCU_GetRevisionID(), LL_DBGMCU_GetDeviceID());
}

u32 Sys_MCU_GetFlashSize(void) {
	return LL_GetFlashSize();
}

bool Sys_CounterCPU_Init(void) {
	if (ARM_CM_DWT_CTRL != 0)  // See if DWT is available
	{
		ARM_CM_DWT_CYCCNT = 0;
		ARM_CM_DEMCR |= CoreDebug_DEMCR_TRCENA_Msk;
		ARM_CM_DWT_CTRL |= DWT_CTRL_CYCCNTENA_Msk;
		return true;
	}

	return false;
}

u32 Sys_CounterCPU_Get(void) {
	return ARM_CM_DWT_CYCCNT;
}

void Sys_MCU_Reset(void) {
	NVIC_SystemReset();
}

void Sys_NVIC_SetPrioEnable(IRQn_Type irq, u16 prio) {
	NVIC_SetPriority(irq, prio);
	NVIC_EnableIRQ(irq);
}

void Sys_NVIC_Disable(IRQn_Type irq) {
	NVIC_SetPriority(irq, 0);
	NVIC_DisableIRQ(irq);
}
