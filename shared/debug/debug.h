#ifndef __DEBUG_H
#define __DEBUG_H

#include "debug_cfg.h"
#include "main.h"
// clang-format off

#define ESC_SYM_BACKSPACE			'\b'
#define ESC_SYM_DELETE				0x7f
#define ESC_SYM_TAB					'\t'
#define ESC_SYM_EXIT				0x03 //ctrl + c

#define ESC_END_LINE				"\r\n"

#define ESC_SAVE_CURSOR				"\e7"
#define ESC_RESTORE_CURSOR			"\e8"
#define ESC_CLEAR_RIGHT_FROM_CURS	"\e[0K"
#define ESC_RIGHT_ARROW				"\e[C"
#define ESC_LEFT_ARROW				"\e[D"

#define ESC_PRE_ARROW_SYM			'['
#define ESC_ARROW_UP_SYM			'A'
#define ESC_ARROW_DOWN_SYM			'B'
#define ESC_ARROW_RIGHT_SYM			'C'
#define ESC_ARROW_LEFT_SYM			'D'

#define ESC_COLOR_RED				"\e[31m"
#define ESC_COLOR_GREEN				"\e[32m"
#define ESC_COLOR_YELLOW			"\e[33m"
#define ESC_COLOR_BLUE				"\e[34m"
#define ESC_COLOR_MAGENTA			"\e[35m"
#define ESC_COLOR_CYAN				"\e[36m"
#define ESC_COLOR_WHITE				"\e[37m"
#define ESC_RESET_STYLE				"\e[0m"
#define ECS_CLR_SCREEN				"\e[1;1H\e[2J"

#define ECS_SET_MODE_BOLD			"\e[1m"
#define ECS_RESET_MODE_BOLD			"\e[22m"
#define ECS_SET_MODE_ITALIC			"\e[3m"
#define ECS_RESET_MODE_ITALIC		"\e[23m"

#define DEBUG_LOG_STR	"[%s] [%lu] [fi: %s, th: %s, fn: %s, ln: %u]:\r\n\t  " // [Time/Date] [TICK_CNT] [FILENAME, TASK, FUNCTION, LINE]

#define DEBUG_LVL_TABLE()\
X_ENTRY(LOG_LVL_TRACE,		"  TRACE", ESC_COLOR_CYAN, 		"🟪") \
X_ENTRY(LOG_LVL_DEBUG,		"  DEBUG", ESC_COLOR_BLUE,		"🟦") \
X_ENTRY(LOG_LVL_INFO,		"   INFO", ESC_COLOR_GREEN,		"🟩") \
X_ENTRY(LOG_LVL_WARNING,	"   WARN", ESC_COLOR_YELLOW,	"🟨") \
X_ENTRY(LOG_LVL_ERROR,		"  ERROR", ESC_COLOR_RED,		"🟧") \
X_ENTRY(LOG_LVL_CRITIC,		" CRITIC", ESC_COLOR_RED,		"🟥") \
X_ENTRY(LOG_LVL_DISABLE,	"DISABLE", ESC_COLOR_WHITE,		"⬛") \

#define X_ENTRY(lvl, lvl_str, lvl_color, lvl_emoji) lvl,
typedef enum {
	DEBUG_LVL_TABLE()
} LOG_LVL_t;
#undef X_ENTRY

typedef struct {
	char* Str;
	char* Color;
	char* Emoji;
} LogLvl_t;

typedef struct {
	char* Ptr;
	u32 Len;
} DebugMsg_t;

#define DEBUG_PRINT_DIRECT(_f_, ...)				do { \
														char _dbg_str[512] = ""; \
														snprintf(_dbg_str, sizeof(_dbg_str), (_f_), ##__VA_ARGS__); \
														Debug_TransmitBuff(_dbg_str, strlen(_dbg_str)); \
													} while(0)

#define DEBUG_PRINT_DIRECT_NL(_f_, ...)				do { \
														DEBUG_PRINT_DIRECT(_f_ "\r\n", ##__VA_ARGS__); \
													} while(0)

#define DEBUG_PRINT(_f_, ...)						do { \
														printf((_f_), ##__VA_ARGS__); \
														fflush(stdout); \
													} while(0)

#define DEBUG_COLOR_PRINT(c, _f_, ...)				do { \
														DEBUG_PRINT(c _f_ ESC_RESET_STYLE, ##__VA_ARGS__); \
													} while(0)

#define DEBUG_PRINT_NL(_f_, ...)					do { \
														DEBUG_PRINT(_f_ "\r\n", ##__VA_ARGS__); \
													} while(0)

#define DEBUG_COLOR_PRINT_NL(c, _f_, ...)			do { \
														DEBUG_PRINT_NL(c _f_ ESC_RESET_STYLE, ##__VA_ARGS__); \
													} while(0)

#define DEBUG_FUNC_ENTER_PRINT()					do { \
														DEBUG_PRINT("%s(): ENTER\r\n", __FUNCTION__); \
													} while(0)

#define DEBUG_EXEC_TIME_START()						u64 _t0 = DBG_TIMER_MICROSEC_GET()

#define DEBUG_EXEC_TIME_STOP_PRINT()				float _dt = (float)(DBG_TIMER_MICROSEC_GET() - _t0); \
													DEBUG_PRINT("%s(): DURATION %.3fms\r\n", \
														__FUNCTION__, _dt / 1000.0f)

#define DEBUG_FUNC_EXIT_PRINT(rs)					do { \
														DEBUG_PRINT("%s(): EXIT with [%s]\r\n\r\n", \
															__FUNCTION__, RetState_GetStr(rs)); \
													} while(0)

#define DEBUG_LOG_PRINT(_f_, ...)					do { \
														char _td_str[TD_STR_SIZE] = ""; \
														DBG_TIME_DATE_STR_GET(_td_str, TD_STR_SIZE); \
														char _tn_str[TASK_NAME_STR_SIZE] = ""; \
														DBG_TASK_NAME_STR_GET(_tn_str, TASK_NAME_STR_SIZE); \
														DEBUG_PRINT(DEBUG_LOG_STR _f_ "\r\n", \
																_td_str, \
																DBG_TIMER_MILLISEC_GET(), \
																DBG_FILENAME, \
																_tn_str, \
																__FUNCTION__, \
																__LINE__, \
																##__VA_ARGS__); \
													} while(0)
													
#define DEBUG_LOG_COLOR_PRINT(c, _f_, ...)			do { \
														DEBUG_LOG_PRINT(c _f_ ESC_RESET_STYLE, ##__VA_ARGS__); \
													} while(0)

#define DEBUG_LOG_LVL_PRINT(l, _f_, ...)			do { \
														if(l >= LOG_LVL_DISABLE) \
															break; \
														if(l >= Debug_LogLvl_Get()) \
														{ \
															char _td_str[TD_STR_SIZE] = ""; \
															DBG_TIME_DATE_STR_GET(_td_str, TD_STR_SIZE); \
															char _tn_str[TASK_NAME_STR_SIZE] = ""; \
															DBG_TASK_NAME_STR_GET(_tn_str, TASK_NAME_STR_SIZE); \
															DEBUG_PRINT("[%s%s%s] " ESC_COLOR_CYAN DEBUG_LOG_STR ESC_RESET_STYLE _f_ "\r\n", \
																	Debug_LogLvl_GetColor(l), \
																	Debug_LogLvl_GetStr(l), \
																	ESC_RESET_STYLE, \
																	_td_str, \
																	DBG_TIMER_MILLISEC_GET(), \
																	DBG_FILENAME, \
																	_tn_str, \
																	__FUNCTION__, \
																	__LINE__, \
																	##__VA_ARGS__); \
														} \
													} while(0)

#define DEBUG_LOG_LVL_PRINT_LOCAL(l, _f_, ...)		do { \
														if(l >= LOG_LVL_DISABLE) \
															break; \
														char _td_str[TD_STR_SIZE] = ""; \
														DBG_TIME_DATE_STR_GET(_td_str, TD_STR_SIZE); \
														char _tn_str[TASK_NAME_STR_SIZE] = ""; \
														DBG_TASK_NAME_STR_GET(_tn_str, TASK_NAME_STR_SIZE); \
														DEBUG_PRINT("[%s%s%s] " ESC_COLOR_CYAN DEBUG_LOG_STR ESC_RESET_STYLE _f_ "\r\n", \
																Debug_LogLvl_GetColor(l), \
																Debug_LogLvl_GetStr(l), \
																ESC_RESET_STYLE, \
																_td_str, \
																DBG_TIMER_MILLISEC_GET(), \
																DBG_FILENAME, \
																_tn_str, \
																__FUNCTION__, \
																__LINE__, \
																##__VA_ARGS__); \
													} while(0)
// clang-format on

void Debug_Init(void);
bool Debug_HardwareIsInit(void);
bool Debug_SendChar(char ch, u32 waitTmo);
bool Debug_SendCharFromISR(char ch, BaseType_t* pWoken);
bool Debug_SendString(char* pStr, u32 waitTmo);
bool Debug_TransmitBuff(char* pBuff, u32 size);
char Debug_ReceiveSymbol(u32 delay);

void Debug_LogLvl_Set(LOG_LVL_t lvl);
LOG_LVL_t Debug_LogLvl_Get(void);
char Debug_LogLvl_GetChar(LOG_LVL_t lvl);
char* Debug_LogLvl_GetStr(LOG_LVL_t lvl);
char* Debug_LogLvl_GetColor(LOG_LVL_t lvl);
char* Debug_LogLvl_GetEmoji(LOG_LVL_t lvl);

void Debug_PrintMainInfo(void);
void Debug_PrintSysInfo(void);

void Debug_CreateHardFault(void);
void Debug_LedOff(void);
void Debug_LedOn(void);
void Debug_LedToggle(void);

TaskHandle_t DebugSend_GetTaskHandle(void);
void FreeRTOS_DebugSend_InitComponents(bool resources, bool tasks);

#endif /* __DEBUG_H */
