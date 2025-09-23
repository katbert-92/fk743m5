#ifndef __PLATFORM_INT_CFG_M0_H
#define __PLATFORM_INT_CFG_M0_H
// clang-format off

#define NVIC_IRQ_PRIO_0							0
#define NVIC_IRQ_PRIO_TIM_DELAY					NVIC_IRQ_PRIO_0

#define NVIC_IRQ_PRIO_1							(NVIC_IRQ_PRIO_0 + 1)

#define NVIC_IRQ_PRIO_2							(NVIC_IRQ_PRIO_1 + 1)

#define NVIC_IRQ_PRIO_3							(NVIC_IRQ_PRIO_2 + 1)

#define NVIC_IRQ_PRIO_4							(NVIC_IRQ_PRIO_3 + 1)
#define NVIC_IRQ_PRIO_EMMC						NVIC_IRQ_PRIO_4

/* Interrupts below are masked after entering critical section -
 * see configLIBRARY_MAX_SYSCALL_INTERRUPT_PRIORITY */

#define NVIC_IRQ_PRIO_5							(NVIC_IRQ_PRIO_4 + 1)
#define NVIC_IRQ_PRIO_USART_DEBUG				NVIC_IRQ_PRIO_5
#define NVIC_IRQ_PRIO_USB_HS					NVIC_IRQ_PRIO_5

#define NVIC_IRQ_PRIO_6							(NVIC_IRQ_PRIO_5 + 1)

#define NVIC_IRQ_PRIO_7							(NVIC_IRQ_PRIO_6 + 1)

#define NVIC_IRQ_PRIO_8							(NVIC_IRQ_PRIO_7 + 1)

#define NVIC_IRQ_PRIO_9							(NVIC_IRQ_PRIO_8 + 1)

#define NVIC_IRQ_PRIO_10						(NVIC_IRQ_PRIO_9 + 1)

#define NVIC_IRQ_PRIO_11						(NVIC_IRQ_PRIO_10 + 1)

#define NVIC_IRQ_PRIO_12						(NVIC_IRQ_PRIO_11 + 1)

#define NVIC_IRQ_PRIO_13						(NVIC_IRQ_PRIO_12 + 1)

#define NVIC_IRQ_PRIO_14						(NVIC_IRQ_PRIO_13 + 1)

#define NVIC_IRQ_PRIO_MAX						NVIC_IRQ_PRIO_14

#endif /* __PLATFORM_INT_CFG_M0_H */
