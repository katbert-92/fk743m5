#include "bkp_storage.h"
#include "dbg_cfg.h"
#include "debug.h"
#include "device_name.h"
#include "platform.h"

#ifndef DEBUG_DEF_LOG_LVL
#define DEBUG_DEF_LOG_LVL LOG_LVL_INFO
#endif /* DEBUG_DEF_LOG_LVL */

#define X_ENTRY(lvl, lvl_str, lvl_color, lvl_emoji) \
	{                                               \
		.Str   = lvl_str,                           \
		.Color = lvl_color,                         \
		.Emoji = lvl_emoji,                         \
	},
static const LogLvl_t LogLvl[] = {DEBUG_LVL_TABLE()};
#undef X_ENTRY

static LOG_LVL_t DebugLogLvl = DEBUG_DEF_LOG_LVL;

void Debug_LogLvl_Set(LOG_LVL_t lvl) {
	DebugLogLvl = lvl;
}

LOG_LVL_t Debug_LogLvl_Get(void) {
	return DebugLogLvl;
}

char Debug_LogLvl_GetChar(LOG_LVL_t lvl) {
	return LogLvl[lvl].Str[0];
}

char* Debug_LogLvl_GetStr(LOG_LVL_t lvl) {
	return LogLvl[lvl].Str;
}

char* Debug_LogLvl_GetColor(LOG_LVL_t lvl) {
	return LogLvl[lvl].Color;
}

char* Debug_LogLvl_GetEmoji(LOG_LVL_t lvl) {
	return LogLvl[lvl].Emoji;
}

void Debug_PrintMainInfo(void) {
	DEBUG_PRINT(ESC_END_LINE ESC_END_LINE ESC_COLOR_RED
				"+++++++++ Device started +++++++++" ESC_RESET_STYLE ESC_END_LINE);

	DEBUG_PRINT(ESC_END_LINE "Main info: " ESC_END_LINE);
	DEBUG_PRINT(" * Project: " ESC_COLOR_CYAN "%s" ESC_RESET_STYLE ESC_END_LINE, FW_PROJECT);
	DEBUG_PRINT(" * Device name: " ESC_COLOR_CYAN "%s" ESC_RESET_STYLE ESC_END_LINE,
				DeviceName_GetPtr());
	DEBUG_PRINT(" * Fw/type: " ESC_COLOR_CYAN "%s" ESC_RESET_STYLE ESC_END_LINE, FW_TYPE);
	DEBUG_PRINT(" * Fw/version: " ESC_COLOR_CYAN "%d.%d.%d" ESC_RESET_STYLE ESC_END_LINE,
				FW_VER_MAJOR, FW_VER_MINOR, FW_VER_BUILD);
	DEBUG_PRINT(" * Fw/tag: " ESC_COLOR_CYAN "%s" ESC_RESET_STYLE ESC_END_LINE, FW_TAG);
	DEBUG_PRINT(" * Hw/platform: " ESC_COLOR_CYAN "%s" ESC_RESET_STYLE ESC_END_LINE, FW_PLATFORM);
	DEBUG_PRINT(" * Fw/BSP: " ESC_COLOR_CYAN "%s" ESC_RESET_STYLE ESC_END_LINE, FW_BSP);
	DEBUG_PRINT(" * Fw/comment: " ESC_COLOR_CYAN "%s" ESC_RESET_STYLE ESC_END_LINE, FW_COMMENT);
	DEBUG_PRINT(" * GIT Hash: " ESC_COLOR_CYAN "%s" ESC_RESET_STYLE ESC_END_LINE, FW_GIT_HASH);
	DEBUG_PRINT(" * GIT Branch: " ESC_COLOR_CYAN "%s" ESC_RESET_STYLE ESC_END_LINE, FW_GIT_BRANCH);
	DEBUG_PRINT(" * Build date/time: " ESC_COLOR_CYAN "%s" ESC_RESET_STYLE ESC_END_LINE,
				(__DATE__ " " __TIME__));

	DEBUG_PRINT(ESC_RESET_STYLE);
}

void Debug_PrintSysInfo(void) {
	char tmpBuff[32];

	DEBUG_PRINT(ESC_END_LINE "Sys info: " ESC_END_LINE);
	Pl_UID_GetStrAndPtr(tmpBuff);
	DEBUG_PRINT(" * MCU UID: " ESC_COLOR_CYAN "%s" ESC_RESET_STYLE ESC_END_LINE, tmpBuff);
	Pl_CPU_GetStrAndPtr(tmpBuff);
	DEBUG_PRINT(" * MCU DevID/RevID: " ESC_COLOR_CYAN "%s" ESC_RESET_STYLE ESC_END_LINE, tmpBuff);
	DEBUG_PRINT(" * MCU Flash (KBytes): " ESC_COLOR_CYAN "%d" ESC_RESET_STYLE ESC_END_LINE,
				Pl_MCU_GetFlashSize());
	DEBUG_PRINT(
		" * MCU Main Clocks (Hz): " ESC_COLOR_CYAN
		"SYSCLK: %u, HCLK: %u, APB1: %u, APB2: %u, APB3: %u, APB4: %u" ESC_RESET_STYLE ESC_END_LINE,
		Pl_SysClk.SYSCLK, Pl_SysClk.HCLK, Pl_SysClk.APB1, Pl_SysClk.APB2, Pl_SysClk.APB3,
		Pl_SysClk.APB4);
	DEBUG_PRINT(" * MCU PLL1 Clocks (Hz): " ESC_COLOR_CYAN
				"P: %u, Q: %u, R: %u" ESC_RESET_STYLE ESC_END_LINE,
				Pl_SysClk.PLL1.P, Pl_SysClk.PLL1.Q, Pl_SysClk.PLL1.R);
	DEBUG_PRINT(" * MCU PLL2 Clocks (Hz): " ESC_COLOR_CYAN
				"P: %u, Q: %u, R: %u" ESC_RESET_STYLE ESC_END_LINE,
				Pl_SysClk.PLL2.P, Pl_SysClk.PLL2.Q, Pl_SysClk.PLL2.R);
	DEBUG_PRINT(" * MCU PLL3 Clocks (Hz): " ESC_COLOR_CYAN
				"P: %u, Q: %u, R: %u" ESC_RESET_STYLE ESC_END_LINE,
				Pl_SysClk.PLL3.P, Pl_SysClk.PLL3.Q, Pl_SysClk.PLL3.R);
	DEBUG_PRINT(" * MCU Reset Flag: " ESC_COLOR_CYAN "%s" ESC_RESET_STYLE ESC_END_LINE,
				Pl_GetRstFlagStr());
	DEBUG_PRINT(" * MCU Fault Addr: " ESC_COLOR_CYAN "0x%08x" ESC_RESET_STYLE ESC_END_LINE,
				BkpStorage_GetValue(BKP_KEY_SYS_FAULT_EXEPTION_ADDR));

	DEBUG_PRINT(ESC_RESET_STYLE);
}

void Debug_CreateHardFault(void) {
	//Exactly unaligned instruction address
	((void (*)(void))0x01)();
}

void Debug_LedOff(void) {
	Pl_LedDebug_SetState(0);
}

void Debug_LedOn(void) {
	Pl_LedDebug_SetState(1);
}

void Debug_LedToggle(void) {
	Pl_LedDebug_SetState(-1);
}
