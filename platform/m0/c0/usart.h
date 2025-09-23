#ifndef __USART_H
#define __USART_H

#include "main.h"
#include "platform.h"
#include "platform_inc_m0.h"

#define USART_DEBUG_IRQ	   USART1_IRQn

USART_TypeDef* USART_Debug_GetInterface(void);

bool USART_Debug_Init(u32 baudRate, Pl_UartExtra_RxClbk_t pRxClbk);
bool USART_Debug_TxData(u8* pBuff, u16 buffSize, u32 tmo);
u8 USART_Debug_RxByte(void);

#endif /* __USART_H */
