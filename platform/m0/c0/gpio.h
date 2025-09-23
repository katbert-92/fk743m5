#ifndef __GPIO_H
#define __GPIO_H

#include "main.h"
#include "platform.h"
#include "platform_inc_m0.h"

#define GPIO_OUTPUT_ACTION_TABLE()            \
	X_ENTRY(GPIO_RST, LL_GPIO_ResetOutputPin) \
	X_ENTRY(GPIO_SET, LL_GPIO_SetOutputPin)   \
	X_ENTRY(GPIO_REV, LL_GPIO_TogglePin)

#define X_ENTRY(state, func) state,
typedef enum { GPIO_OUTPUT_ACTION_TABLE() GPIO_OUTPUT_ACTION_ENUM_SIZE } GPIO_ACTION_t;
#undef X_ENTRY

void GPIO_MCO_Init(void);

void GPIO_LedDebug_Init(void);
void GPIO_LedDebug_SetState(GPIO_ACTION_t action);

void GPIO_UartDebug_Init(void);

void GPIO_Emmc_Init(void);

void GPIO_Ospi_Init(void);

#endif /* __GPIO_H */
