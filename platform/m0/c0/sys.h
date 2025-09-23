#ifndef __SYS_H
#define __SYS_H

#include "main.h"
#include "platform.h"
#include "platform_inc_m0.h"

#define SYS_RST_SRC_TABLE()                    \
	X_ENTRY(SYS_RESET_FLAG_UNKNOWN, "UNKNOWN") \
	X_ENTRY(SYS_RESET_FLAG_WWDG, "WWDG")       \
	X_ENTRY(SYS_RESET_FLAG_IWDG, "IWDG")       \
	X_ENTRY(SYS_RESET_FLAG_SOFT, "SOFT")       \
	X_ENTRY(SYS_RESET_FLAG_POR, "POR")         \
	X_ENTRY(SYS_RESET_FLAG_PIN, "PIN")         \
	X_ENTRY(SYS_RESET_FLAG_BOR, "BOR")         \
	X_ENTRY(SYS_RESET_FLAG_D1, "D1")           \
	X_ENTRY(SYS_RESET_FLAG_D2, "D2")           \
	X_ENTRY(SYS_RESET_FLAG_CPU, "CPU")

#define X_ENTRY(flag, flag_str) flag,
typedef enum { SYS_RST_SRC_TABLE() } SYS_RESET_FLAG_t;
#undef X_ENTRY

const char* Sys_ResetFlag_GetStr(void);
SYS_RESET_FLAG_t Sys_ResetFlag_Get(void);
void Sys_ResetFlag_Clear(void);

bool Sys_LSI_IsReadyAndClkSrc(void);
bool Sys_LSE_IsReadyAndClkSrc(void);
bool Sys_RealTimeClock_Config(void);
void Sys_MainClock_Config(void);
void Sys_PerifClock_Config(void);
Pl_Pll_t Sys_PLL_GetClockFreq(u32 pllNum);

u32* Sys_UID_GetStrAndPtr(char* pDst);
void Sys_CPU_GetStrAndPtr(char* pDst);
u32 Sys_MCU_GetFlashSize(void);
bool Sys_CounterCPU_Init(void);
u32 Sys_CounterCPU_Get(void);
void Sys_MCU_Reset(void);
void Sys_NVIC_SetPrioEnable(IRQn_Type irq, u16 prio);
void Sys_NVIC_Disable(IRQn_Type irq);

#endif /* __SYS_H */
