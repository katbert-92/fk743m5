#include "gpio.h"

// clang-format off
#define LED_DBG_PORT				GPIOC
#define LED_DBG_PIN					LL_GPIO_PIN_13

#define DEBUG_TX_RX_PORT			GPIOA
#define DEBUG_TX_PIN				LL_GPIO_PIN_9
#define DEBUG_RX_PIN				LL_GPIO_PIN_10
#define DEBUG_AF					LL_GPIO_AF_7


#define EMMC_D0_PORT				GPIOC
#define EMMC_D0_PIN					LL_GPIO_PIN_8
#define EMMC_D0_AF					LL_GPIO_AF_12
#define EMMC_D1_PORT				GPIOC
#define EMMC_D1_PIN					LL_GPIO_PIN_9
#define EMMC_D1_AF					LL_GPIO_AF_12
#define EMMC_D2_PORT				GPIOC
#define EMMC_D2_PIN					LL_GPIO_PIN_10
#define EMMC_D2_AF					LL_GPIO_AF_12
#define EMMC_D3_PORT				GPIOC
#define EMMC_D3_PIN					LL_GPIO_PIN_11
#define EMMC_D3_AF					LL_GPIO_AF_12
#define EMMC_D4_PORT				GPIOB
#define EMMC_D4_PIN					LL_GPIO_PIN_8
#define EMMC_D4_AF					LL_GPIO_AF_12
#define EMMC_D5_PORT				GPIOB
#define EMMC_D5_PIN					LL_GPIO_PIN_9
#define EMMC_D5_AF					LL_GPIO_AF_12
#define EMMC_D6_PORT				GPIOC
#define EMMC_D6_PIN					LL_GPIO_PIN_6
#define EMMC_D6_AF					LL_GPIO_AF_12
#define EMMC_D7_PORT				GPIOC
#define EMMC_D7_PIN					LL_GPIO_PIN_7
#define EMMC_D7_AF					LL_GPIO_AF_12
#define EMMC_CK_PORT				GPIOC
#define EMMC_CK_PIN					LL_GPIO_PIN_12
#define EMMC_CK_AF					LL_GPIO_AF_12
#define EMMC_CMD_PORT				GPIOD
#define EMMC_CMD_PIN				LL_GPIO_PIN_2
#define EMMC_CMD_AF					LL_GPIO_AF_12

#define EXT_MEM_IO0_PORT			GPIOF
#define EXT_MEM_IO0_PIN				LL_GPIO_PIN_8
#define EXT_MEM_IO0_AF				LL_GPIO_AF_10
#define EXT_MEM_IO1_PORT			GPIOF
#define EXT_MEM_IO1_PIN				LL_GPIO_PIN_9
#define EXT_MEM_IO1_AF				LL_GPIO_AF_10
#define EXT_MEM_IO2_PORT			GPIOF
#define EXT_MEM_IO2_PIN				LL_GPIO_PIN_7
#define EXT_MEM_IO2_AF				LL_GPIO_AF_10
#define EXT_MEM_IO3_PORT			GPIOF
#define EXT_MEM_IO3_PIN				LL_GPIO_PIN_6
#define EXT_MEM_IO3_AF				LL_GPIO_AF_10
#define EXT_MEM_CLK_PORT			GPIOF
#define EXT_MEM_CLK_PIN				LL_GPIO_PIN_10
#define EXT_MEM_CLK_AF				LL_GPIO_AF_9
#define EXT_MEM_NCS_PORT			GPIOG
#define EXT_MEM_NCS_PIN				LL_GPIO_PIN_6
#define EXT_MEM_NCS_AF				LL_GPIO_AF_10
// clang-format on

// STM32H723 pin alternate functions: DS, page 72

typedef void (*GPIO_Action_FuncPtr_t)(GPIO_TypeDef*, u32);

#define X_ENTRY(state, func) func,
static const GPIO_Action_FuncPtr_t GPIO_FuncList[] = {GPIO_OUTPUT_ACTION_TABLE()};
#undef X_ENTRY


void GPIO_MCO_Init(void) {
	LL_RCC_ConfigMCO(LL_RCC_MCO1SOURCE_PLL1QCLK, LL_RCC_MCO1_DIV_15);  //2M
	LL_GPIO_SetPinMode(GPIOA, LL_GPIO_PIN_8, LL_GPIO_MODE_ALTERNATE);
	LL_GPIO_SetPinOutputType(GPIOA, LL_GPIO_PIN_8, LL_GPIO_OUTPUT_PUSHPULL);
	LL_GPIO_SetPinSpeed(GPIOA, LL_GPIO_PIN_8, LL_GPIO_SPEED_FREQ_VERY_HIGH);
	LL_GPIO_SetPinPull(GPIOA, LL_GPIO_PIN_8, LL_GPIO_PULL_NO);
	LL_GPIO_SetAFPin_8_15(GPIOA, LL_GPIO_PIN_8, LL_GPIO_AF_0);

	LL_RCC_ConfigMCO(LL_RCC_MCO2SOURCE_SYSCLK, LL_RCC_MCO2_DIV_15);	 //10M
	LL_GPIO_SetPinMode(GPIOC, LL_GPIO_PIN_9, LL_GPIO_MODE_ALTERNATE);
	LL_GPIO_SetPinOutputType(GPIOC, LL_GPIO_PIN_9, LL_GPIO_OUTPUT_PUSHPULL);
	LL_GPIO_SetPinSpeed(GPIOC, LL_GPIO_PIN_9, LL_GPIO_SPEED_FREQ_VERY_HIGH);
	LL_GPIO_SetPinPull(GPIOC, LL_GPIO_PIN_9, LL_GPIO_PULL_NO);
	LL_GPIO_SetAFPin_8_15(GPIOC, LL_GPIO_PIN_9, LL_GPIO_AF_0);
}

void GPIO_LedDebug_Init(void) {
	LL_GPIO_InitTypeDef GPIO_InitStruct;
	LL_GPIO_StructInit(&GPIO_InitStruct);

	GPIO_InitStruct.Pin		   = LED_DBG_PIN;
	GPIO_InitStruct.Mode	   = LL_GPIO_MODE_OUTPUT;
	GPIO_InitStruct.Speed	   = LL_GPIO_SPEED_FREQ_LOW;
	GPIO_InitStruct.OutputType = LL_GPIO_OUTPUT_PUSHPULL;
	GPIO_InitStruct.Pull	   = LL_GPIO_PULL_NO;
	LL_GPIO_Init(LED_DBG_PORT, &GPIO_InitStruct);
}

void GPIO_LedDebug_SetState(GPIO_ACTION_t action) {
	GPIO_FuncList[action](LED_DBG_PORT, LED_DBG_PIN);
}

void GPIO_UartDebug_Init(void) {
	LL_GPIO_InitTypeDef GPIO_InitStruct;
	LL_GPIO_StructInit(&GPIO_InitStruct);

	GPIO_InitStruct.Pin		   = DEBUG_TX_PIN | DEBUG_RX_PIN;
	GPIO_InitStruct.Mode	   = LL_GPIO_MODE_ALTERNATE;
	GPIO_InitStruct.Speed	   = LL_GPIO_SPEED_FREQ_MEDIUM;
	GPIO_InitStruct.OutputType = LL_GPIO_OUTPUT_PUSHPULL;
	GPIO_InitStruct.Pull	   = LL_GPIO_PULL_UP;
	GPIO_InitStruct.Alternate  = DEBUG_AF;
	LL_GPIO_Init(DEBUG_TX_RX_PORT, &GPIO_InitStruct);
}


void GPIO_Emmc_Init(void) {
	LL_GPIO_InitTypeDef GPIO_InitStruct;
	LL_GPIO_StructInit(&GPIO_InitStruct);

	GPIO_InitStruct.Mode  = LL_GPIO_MODE_ALTERNATE;
	GPIO_InitStruct.Speed = LL_GPIO_SPEED_FREQ_HIGH;
	GPIO_InitStruct.Pull  = LL_GPIO_PULL_UP;

	GPIO_InitStruct.Alternate = EMMC_D0_AF;
	GPIO_InitStruct.Pin		  = EMMC_D0_PIN;
	LL_GPIO_Init(EMMC_D0_PORT, &GPIO_InitStruct);
	GPIO_InitStruct.Alternate = EMMC_D1_AF;
	GPIO_InitStruct.Pin		  = EMMC_D1_PIN;
	LL_GPIO_Init(EMMC_D1_PORT, &GPIO_InitStruct);
	GPIO_InitStruct.Alternate = EMMC_D2_AF;
	GPIO_InitStruct.Pin		  = EMMC_D2_PIN;
	LL_GPIO_Init(EMMC_D2_PORT, &GPIO_InitStruct);
	GPIO_InitStruct.Alternate = EMMC_D3_AF;
	GPIO_InitStruct.Pin		  = EMMC_D3_PIN;
	LL_GPIO_Init(EMMC_D3_PORT, &GPIO_InitStruct);
	GPIO_InitStruct.Alternate = EMMC_D4_AF;
	GPIO_InitStruct.Pin		  = EMMC_D4_PIN;
	LL_GPIO_Init(EMMC_D4_PORT, &GPIO_InitStruct);
	GPIO_InitStruct.Alternate = EMMC_D5_AF;
	GPIO_InitStruct.Pin		  = EMMC_D5_PIN;
	LL_GPIO_Init(EMMC_D5_PORT, &GPIO_InitStruct);
	GPIO_InitStruct.Alternate = EMMC_D6_AF;
	GPIO_InitStruct.Pin		  = EMMC_D6_PIN;
	LL_GPIO_Init(EMMC_D6_PORT, &GPIO_InitStruct);
	GPIO_InitStruct.Alternate = EMMC_D7_AF;
	GPIO_InitStruct.Pin		  = EMMC_D7_PIN;
	LL_GPIO_Init(EMMC_D7_PORT, &GPIO_InitStruct);
	GPIO_InitStruct.Alternate = EMMC_CK_AF;
	GPIO_InitStruct.Pin		  = EMMC_CK_PIN;
	LL_GPIO_Init(EMMC_CK_PORT, &GPIO_InitStruct);
	GPIO_InitStruct.Alternate = EMMC_CMD_AF;
	GPIO_InitStruct.Pin		  = EMMC_CMD_PIN;
	LL_GPIO_Init(EMMC_CMD_PORT, &GPIO_InitStruct);
}

void GPIO_Ospi_Init(void) {
	LL_GPIO_InitTypeDef GPIO_InitStruct;
	LL_GPIO_StructInit(&GPIO_InitStruct);

	GPIO_InitStruct.Mode  = LL_GPIO_MODE_ALTERNATE;
	GPIO_InitStruct.Speed = LL_GPIO_SPEED_FREQ_VERY_HIGH;
	GPIO_InitStruct.Pull  = LL_GPIO_PULL_UP;

	GPIO_InitStruct.Alternate = EXT_MEM_IO0_AF;
	GPIO_InitStruct.Pin		  = EXT_MEM_IO0_PIN;
	LL_GPIO_Init(EXT_MEM_IO0_PORT, &GPIO_InitStruct);
	GPIO_InitStruct.Alternate = EXT_MEM_IO1_AF;
	GPIO_InitStruct.Pin		  = EXT_MEM_IO1_PIN;
	LL_GPIO_Init(EXT_MEM_IO1_PORT, &GPIO_InitStruct);
	GPIO_InitStruct.Alternate = EXT_MEM_IO2_AF;
	GPIO_InitStruct.Pin		  = EXT_MEM_IO2_PIN;
	LL_GPIO_Init(EXT_MEM_IO2_PORT, &GPIO_InitStruct);
	GPIO_InitStruct.Alternate = EXT_MEM_IO3_AF;
	GPIO_InitStruct.Pin		  = EXT_MEM_IO3_PIN;
	LL_GPIO_Init(EXT_MEM_IO3_PORT, &GPIO_InitStruct);
	GPIO_InitStruct.Alternate = EXT_MEM_CLK_AF;
	GPIO_InitStruct.Pin		  = EXT_MEM_CLK_PIN;
	LL_GPIO_Init(EXT_MEM_CLK_PORT, &GPIO_InitStruct);
	GPIO_InitStruct.Alternate = EXT_MEM_NCS_AF;
	GPIO_InitStruct.Pin		  = EXT_MEM_NCS_PIN;
	LL_GPIO_Init(EXT_MEM_NCS_PORT, &GPIO_InitStruct);
}
