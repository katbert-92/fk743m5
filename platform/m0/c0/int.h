#ifndef __INT_H
#define __INT_H

#include "main.h"
#include "platform.h"

void HardFault_SetCallback(Pl_HardFault_Clbk_t hardFault_Clbk);

void NMI_Handler(void);
void HardFault_Handler(void);
void MemManage_Handler(void);
void BusFault_Handler(void);
void UsageFault_Handler(void);
void DebugMon_Handler(void);

/*void SVC_Handler(void);
void PendSV_Handler(void);
void SysTick_Handler(void);*/

#endif /* __INT_H */
