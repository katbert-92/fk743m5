#include "tim.h"

// clang-format off
#define DELAY_TIM					TIM6
#define DELAY_TIM_CLK_EN()			LL_APB1_GRP1_EnableClock(LL_APB1_GRP1_PERIPH_TIM6)
#define DELAY_TIM_IRQ_HDL			TIM6_DAC_IRQHandler
// clang-format on

static Pl_Common_Clbk_t DelayTimerClbk = Pl_Stub_CommonClbk;
volatile static u32 TimDelayOverflowsCnt;

bool TIM_Delay_Init(Pl_Common_Clbk_t pDelayTimerClbk) {
	if (!Pl_IsInit.Sys) {
		PANIC();
		return false;
	}
	ASSIGN_NOT_NULL_VAL_TO_PTR(DelayTimerClbk, pDelayTimerClbk);

	LL_TIM_InitTypeDef TIM_InitStruct;
	LL_TIM_StructInit(&TIM_InitStruct);

	DELAY_TIM_CLK_EN();
	u32 timClkAPB1 = 2 * Pl_SysClk.APB1;

	TIM_InitStruct.Prescaler   = __LL_TIM_CALC_PSC(timClkAPB1, FREQ_1_MHZ);
	TIM_InitStruct.CounterMode = LL_TIM_COUNTERMODE_UP;
	TIM_InitStruct.Autoreload = __LL_TIM_CALC_ARR(timClkAPB1, TIM_InitStruct.Prescaler, FREQ_1_KHZ);
	TIM_InitStruct.ClockDivision	 = LL_TIM_CLOCKDIVISION_DIV1;
	TIM_InitStruct.RepetitionCounter = 0;
	LL_TIM_Init(DELAY_TIM, &TIM_InitStruct);

	LL_TIM_EnableIT_UPDATE(DELAY_TIM);
	LL_TIM_EnableCounter(DELAY_TIM);

	return true;
}

void TIM_Delay_Disable(void) {
	LL_TIM_DisableCounter(DELAY_TIM);
}

void TIM_Delay_Enable(void) {
	LL_TIM_EnableCounter(DELAY_TIM);
}

__INLINE u32 TIM_Delay_GetCnt(void) {
	return LL_TIM_GetCounter(DELAY_TIM);
}

__INLINE u32 TIM_Delay_GetOvrflCnt(void) {
	return TimDelayOverflowsCnt;
}

void DELAY_TIM_IRQ_HDL(void) {
	if (LL_TIM_IsActiveFlag_UPDATE(DELAY_TIM)) {
		DelayTimerClbk();
		TimDelayOverflowsCnt++;
		LL_TIM_ClearFlag_UPDATE(DELAY_TIM);
	}
}
