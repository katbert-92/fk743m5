#ifndef __APP_CFG
#define __APP_CFG

/**
 * @brief Enabling JSMN library
 */
#ifndef JSMN_HEADER
#define JSMN_HEADER
#endif /* JSMN_HEADER */

/**
 * Debug serial interface selection
 */
#define DEBUG_UART_BAUDRATE 921600
#if !defined DEBUG_OUTPUT_THROUGH_UART && !defined DEBUG_OUTPUT_THROUGH_USB
// #define DEBUG_OUTPUT_THROUGH_UART
#define DEBUG_OUTPUT_THROUGH_USB
#endif /* DEBUG_OUTPUT_THROUGH_UART || DEBUG_OUTPUT_THROUGH_USB */

/**
 * @brief Use custom wrapper for memory allocation functions
 */
#ifndef MEM_ALLOC_TRACKER
#define MEM_ALLOC_TRACKER 0
#endif /* MEM_ALLOC_TRACKER */

/**
 * @brief Enable periodic RTOS stack and tasks stack checkout
 */
#ifndef RTOS_ANALYZER
#define RTOS_ANALYZER 0
#endif /* RTOS_ANALYZER */

/**
 * @brief Console via serial interface
 */
#ifndef SHELL_INTERFACE
#define SHELL_INTERFACE 1
#endif /* SHELL_INTERFACE */

/**
 * @brief System periodic health check
 */
#ifndef HEALTH_CHECK
#define HEALTH_CHECK 1
#endif /* HEALTH_CHECK */

#endif /* __APP_CFG */
