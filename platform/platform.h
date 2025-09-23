#ifndef __PLATFORM_H
#define __PLATFORM_H

#include "main.h"

#ifndef PL_NOP
#define PL_NOP() __NOP()
#endif /* PL_NOP */

#ifndef PL_SET_BKPT
#define PL_SET_BKPT(v) __BKPT(v)
#endif /* PL_SET_BKPT */

__STATIC_FORCEINLINE void PL_IrqOn(void) {
	__enable_irq();
}

__STATIC_FORCEINLINE void PL_IrqOff(void) {
	__disable_irq();
}

#ifdef FW_PLATFORM_M0
#define PL_QUICKACCESS_DATA	   __attribute__((section(".QUICK_DATA")))
#define PL_NO_CACHE_DMA_DATA   __attribute__((section(".NO_CACHE_DMA_DATA")))
#define PL_SHELL_HISTORY_DATA  __attribute__((section(".SHELL_HISTORY_DATA")))
#define PL_BKP_STORAGE_DATA	   __attribute__((section(".BKP_STORAGE_DATA")))
#define PL_STORAGE_IN_RAM_DATA __attribute__((section(".STORAGE_RAM_DATA")))
#else /* FW_PLATFORM_M0 */
#define PL_QUICKACCESS_DATA
#define PL_NO_CACHE_DMA_DATA
#define PL_SHELL_HISTORY_DATA
#define PL_BKP_STORAGE_DATA
#define PL_STORAGE_IN_RAM_DATA
#endif /* FW_PLATFORM_M0 */

#ifdef FW_PLATFORM_M0
#define PL_UART_DEF_TMO		   50
#define PL_SD_EMMC_DEF_TMO	   100
#define PL_TRNG_MAX_ITER	   5
#define PL_SDMMC_SECTOR_SIZE   512U
//LL_RTC_BKP_DR31 == 32
#define PL_BKP_STORAGE_MAX_LEN 32
#else /* FW_PLATFORM_M0 */
#warning Check platform selection!
#endif /* FW_PLATFORM_M0 */

typedef void (*Pl_Common_Clbk_t)(void);
typedef void (*Pl_HardFault_Clbk_t)(u32 pcVal);
typedef void (*Pl_Uart_RxClbk_t)(void);
typedef void (*Pl_UartExtra_RxClbk_t)(void*);
typedef void (*Pl_Dma_TxClbk_t)(void);
typedef void (*Pl_Dma_RxClbk_t)(void);
typedef void (*Pl_Spi_TxClbk_t)(void);
typedef void (*Pl_Usb_RxClbk_t)(u8* pBuff, u32 len);
typedef void (*Pl_Exti_Clbk_t)(void);
//TODO add default callbacks to c file

typedef struct {
	bool Sys;
	bool Bsp;
	bool DelayMs;
	bool Crc32;
	bool TrueRand;
	bool LedSys;
	bool SerialDebug;
	bool LsiClk;
	bool LseClk;
	bool Hsi48Clk;
	bool Rtc;
	bool Iwdg;
	bool SerialWireless;
	bool CpuCounter;
	bool Usb;
	bool I2cTouch;
	bool SpiDisplay;
	bool SdEmmc;
	bool TimLedStrip;
	bool I2cVibro;
	bool TimLedDisplay;
} Pl_IsInit_t;

extern Pl_IsInit_t Pl_IsInit;

typedef struct {
	u32 P;
	u32 Q;
	u32 R;
} Pl_Pll_t;

typedef struct {
	u32 SYSCLK;
	u32 HCLK;
	u32 APB1;
	u32 APB2;
	u32 APB3;
	u32 APB4;
	Pl_Pll_t PLL1;
	Pl_Pll_t PLL2;
	Pl_Pll_t PLL3;
} Pl_SysClock_t;

extern Pl_SysClock_t Pl_SysClk;

typedef struct {
	u32 CardType; /*!< Specifies the card Type                         */
	// u32 CardVersion;  /*!< Specifies the card version                      */
	u32 Class;		  /*!< Specifies the class of the card class           */
	u32 RelCardAdd;	  /*!< Specifies the Relative Card Address             */
	u32 BlockNbr;	  /*!< Specifies the Card Capacity in blocks           */
	u32 BlockSize;	  /*!< Specifies one block size in bytes               */
	u32 LogBlockNbr;  /*!< Specifies the Card logical Capacity in blocks   */
	u32 LogBlockSize; /*!< Specifies logical block size in bytes           */
	u32 MfgID;
	char ProdName[6];
	u32 ProdRev;
	u32 ProdSN;
} Pl_SdEmmcInfo_t;

void Pl_Stub_CommonClbk(void);
void Pl_Stub_ParamClbk(void* pVal);
void Pl_Stub_HardFaultClbk(u32 pcVal);

bool Pl_Init(Pl_HardFault_Clbk_t hardFault_Clbk, u32 maxMaskedIntPrio);

u32* Pl_UID_GetStrAndPtr(char* pDst);
void Pl_CPU_GetStrAndPtr(char* pDst);
u32 Pl_MCU_GetFlashSize(void);

void Pl_LedDebug_Init(void);
void Pl_LedDebug_SetState(s32 state);

bool Pl_DelayMs_Init(Pl_Common_Clbk_t pDelayTimerClbk);
void Pl_DelayMs_SuspendTimer(void);
void Pl_DelayMs_ResumeTimer(void);
u32 Pl_DelayMs_GetUsCnt(void);
u32 Pl_DelayMs_GetMsCnt(void);

bool Pl_Crc_Init(void);
void Pl_Crc_Reset(void);
u32 Pl_Crc32_CheckBuff(const u8* pBuff, u32 buffSize);

void Pl_SysCpuCnt_Init(void);
u32 Pl_SysCpuCnt_Get(void);

void Pl_SoftReset(void);
const char* Pl_GetRstFlagStr(void);

bool Pl_TrueRand_Init(void);
bool Pl_TrueRand_GenerateBuff(u32* pBuff, u32 maxNum);
u32 Pl_TrueRand_GenerateOne(void);

bool Pl_DebugUart_Init(u32 baudRate, u8* pRxBuff, u32 rxBuffLen, Pl_UartExtra_RxClbk_t pRxClbk);
bool Pl_DebugUart_SendBuff(u8* pBuff, u32 size);
u8 Pl_DebugUart_ReceiveByte(void);

bool Pl_RTC_Init(bool isInit, u32* pRetRtcStateBits);
bool Pl_RTC_IsReady(void);
u32 Pl_RTC_GetYearOffset(void);
bool Pl_RTC_SetDate(u16 year, u8 month, u8 day);
bool Pl_RTC_SetTime(u8 hour, u8 minute, u8 second);
void Pl_RTC_TimeDate_Get(u8* pHour, u8* pMinute, u8* pSecond, u8* pMonth, u8* pDay, u8* pWeekday,
						 u16* pYear);

u32 Pl_BkpStorage_GetReg(u32 reg);
void Pl_BkpStorage_SetReg(u32 reg, u32 data);

bool Pl_Watchdog_Init(void);
void Pl_Watchdog_ReloadCounter(void);

void Pl_Sys_DebugInit(void);

bool Pl_Wireless_Init(u32 baudRate, u8* pRxBuff, u32 rxBuffLen, Pl_Uart_RxClbk_t pRxClbk);
bool Pl_Wireless_Send(u8* pBuff, u32 size);
u32 Pl_Wireless_GetDataCnt(void);

bool Pl_USB_CDC_Init(Pl_Usb_RxClbk_t pRxClbk_USB, u8* pRxBuff);
RET_STATE_t Pl_USB_CDC_Transmit(char* pBuff, u16 len);
bool Pl_USB_CDC_IsReady(void);
bool Pl_USB_IsClassCDC(void);

bool Pl_USB_MSC_Init(void);
bool Pl_USB_IsClassMSC(void);

bool Pl_USB_DeInit(void);

bool Pl_Emmc_Init(Pl_SdEmmcInfo_t* pSdEmmcInfo);
bool Pl_Emmc_GetDeviceInfo(Pl_SdEmmcInfo_t* pSdEmmcInfo);
bool Pl_Emmc_Read(u8* pData, u32 blockIdx, u32 blockNum);
bool Pl_Emmc_ReadDMA(u8* pData, u32 blockIdx, u32 blockNum);
bool Pl_Emmc_Write(const u8* pData, u32 blockIdx, u32 blockNum);
bool Pl_Emmc_WriteDMA(const u8* pData, u32 blockIdx, u32 blockNum);
bool Pl_Emmc_Erase(u32 startAddr, u32 endAddr);
u32 Pl_Emmc_GetCardState(void);
bool Pl_Emmc_IsCardInTransfer(void);

#endif /* __PLATFORM_H */
