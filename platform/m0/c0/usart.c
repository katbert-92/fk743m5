#include "usart.h"

// clang-format off
#define DEBUG_USART					USART1
#define DEBUG_USART_IRQ_HDL			USART1_IRQHandler
#define DEBUG_USART_CLK_EN()		LL_APB2_GRP1_EnableClock(LL_APB2_GRP1_PERIPH_USART1);
// clang-format on

USART_TypeDef* USART_Debug_GetInterface(void) {
	return DEBUG_USART;
}

static Pl_UartExtra_RxClbk_t RxClbk_Debug = Pl_Stub_ParamClbk;

static bool USART_WaitFlag(USART_TypeDef* pcUsart, u32 tmo,
						   u32 (*flagFunc)(const USART_TypeDef* pcUsart)) {
	u32 endTime = PL_GET_MS_CNT() + tmo;
	while (!flagFunc(pcUsart)) {
		if (PL_GET_MS_CNT() > endTime) {
			PANIC();
			return false;
		}
	}

	return true;
}

static bool USART_TxData(USART_TypeDef* pcUsart, u8* pBuff, u16 buffSize, u32 tmo) {
	if (!USART_WaitFlag(pcUsart, tmo, LL_USART_IsActiveFlag_TXE_TXFNF))
		return false;

	for (u32 buffIdx = 0; buffIdx < buffSize; buffIdx++) {
		LL_USART_TransmitData8(pcUsart, *pBuff++);
		if (!USART_WaitFlag(pcUsart, tmo, LL_USART_IsActiveFlag_TXE))
			return false;
	}

	if (!USART_WaitFlag(pcUsart, tmo, LL_USART_IsActiveFlag_TC))
		return false;

	return true;
}

static u8 USART_RxByte(USART_TypeDef* pcUsart) {
	return LL_USART_ReceiveData8(pcUsart);
}

bool USART_Debug_Init(u32 baudRate, Pl_UartExtra_RxClbk_t pRxClbk) {
	if (!Pl_IsInit.Sys || !Pl_IsInit.DelayMs) {
		PANIC();
		return false;
	}
	ASSIGN_NOT_NULL_VAL_TO_PTR(RxClbk_Debug, pRxClbk);

	DEBUG_USART_CLK_EN();

	LL_USART_InitTypeDef USART_InitStruct;
	LL_USART_StructInit(&USART_InitStruct);

	USART_InitStruct.BaudRate			 = baudRate;
	USART_InitStruct.DataWidth			 = LL_USART_DATAWIDTH_8B;
	USART_InitStruct.StopBits			 = LL_USART_STOPBITS_1;
	USART_InitStruct.Parity				 = LL_USART_PARITY_NONE;
	USART_InitStruct.TransferDirection	 = LL_USART_DIRECTION_TX_RX;
	USART_InitStruct.HardwareFlowControl = LL_USART_HWCONTROL_NONE;
	USART_InitStruct.OverSampling		 = LL_USART_OVERSAMPLING_16;
	USART_InitStruct.PrescalerValue		 = LL_USART_PRESCALER_DIV4;
	LL_USART_Init(DEBUG_USART, &USART_InitStruct);

	LL_USART_ConfigAsyncMode(DEBUG_USART);
	LL_USART_EnableIT_RXNE(DEBUG_USART);

	LL_USART_Enable(DEBUG_USART);
	while (!LL_USART_IsActiveFlag_TEACK(DEBUG_USART) || !LL_USART_IsActiveFlag_REACK(DEBUG_USART)) {
	}

	return true;
}

bool USART_Debug_TxData(u8* pBuff, u16 buffSize, u32 tmo) {
	return USART_TxData(DEBUG_USART, pBuff, buffSize, tmo);
}

u8 USART_Debug_RxByte(void) {
	return USART_RxByte(DEBUG_USART);
}

void DEBUG_USART_IRQ_HDL(void) {
	if (LL_USART_IsEnabledIT_RXNE_RXFNE(DEBUG_USART) &&
		LL_USART_IsActiveFlag_RXNE_RXFNE(DEBUG_USART)) {
		// LL_USART_ClearFlag_RXNE(DEBUG_USART);

		u8 byte = USART_Debug_RxByte();
		RxClbk_Debug((void*)&byte);
	}

	if (LL_USART_IsEnabledIT_IDLE(DEBUG_USART) && LL_USART_IsActiveFlag_IDLE(DEBUG_USART)) {
		LL_USART_ClearFlag_IDLE(DEBUG_USART);
	}

	if (LL_USART_IsEnabledIT_ERROR(DEBUG_USART)) {
		if (LL_USART_IsActiveFlag_ORE(DEBUG_USART)) {
			LL_USART_ClearFlag_ORE(DEBUG_USART);
			PANIC();
		}

		if (LL_USART_IsActiveFlag_NE(DEBUG_USART)) {
			LL_USART_ClearFlag_NE(DEBUG_USART);
			PANIC();
		}

		if (LL_USART_IsActiveFlag_FE(DEBUG_USART)) {
			LL_USART_ClearFlag_FE(DEBUG_USART);
			// PANIC();
		}
	}

	if (LL_USART_IsEnabledIT_PE(DEBUG_USART) && LL_USART_IsActiveFlag_PE(DEBUG_USART)) {
		LL_USART_ClearFlag_PE(DEBUG_USART);
		PANIC();
	}
}
