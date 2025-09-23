#include "debug.h"
#include "platform.h"
#include "stringlib.h"
#include "wsh_shell.h"

/* clang-format off */
#define CMD_DEBUG_LOG_OPT_TABLE() \
X_CMD_ENTRY(CMD_DEBUG_LOG_OPT_HELP, WSH_SHELL_OPT_HELP()) \
X_CMD_ENTRY(CMD_DEBUG_LOG_OPT_DEF, WSH_SHELL_OPT_NO(WSH_SHELL_OPT_ACCESS_ANY)) \
X_CMD_ENTRY(CMD_DEBUG_LOG_OPT_SET, WSH_SHELL_OPT_STR(WSH_SHELL_OPT_ACCESS_WRITE, "-l", "--lvl", "Set log level")) \
X_CMD_ENTRY(CMD_DEBUG_LOG_OPT_QUIT, WSH_SHELL_OPT_WO_PARAM(WSH_SHELL_OPT_ACCESS_ANY, "-q", "--quit", "Deactivate log mode")) \
X_CMD_ENTRY(CMD_DEBUG_LOG_OPT_END, WSH_SHELL_OPT_END())
/* clang-format on */

#define X_CMD_ENTRY(en, m) en,
typedef enum { CMD_DEBUG_LOG_OPT_TABLE() CMD_DEBUG_LOG_OPT_ENUM_SIZE } CMD_DEBUG_LOG_OPT_t;
#undef X_CMD_ENTRY

#define X_CMD_ENTRY(enum, opt) {enum, opt},
WshShellOption_t DebugOptArr[] = {CMD_DEBUG_LOG_OPT_TABLE()};
#undef X_CMD_ENTRY

static WSH_SHELL_RET_STATE_t shell_cmd_debug_log(const WshShellCmd_t* pcCmd, WshShell_Size_t argc,
												 const char* pArgv[], void* pCtx) {
	if ((argc > 0 && pArgv == NULL) || pcCmd == NULL)
		return WSH_SHELL_RET_STATE_ERROR;

	LOG_LVL_t reqLvl = LOG_LVL_DISABLE;

	WshShell_Size_t tokenPos = 0;
	while (tokenPos < argc) {
		WshShellOption_Context_t optCtx = WshShellCmd_ParseOpt(pcCmd, argc, pArgv, &tokenPos);
		if (optCtx.Option == NULL)
			return WSH_SHELL_RET_STATE_ERR_EMPTY;

		switch (optCtx.Option->ID) {
			case CMD_DEBUG_LOG_OPT_HELP:
			case CMD_DEBUG_LOG_OPT_DEF:
				WshShellCmd_PrintOptionsOverview(pcCmd);
				return WSH_SHELL_RET_STATE_SUCCESS;

			case CMD_DEBUG_LOG_OPT_SET: {
				char lvlStr = '\0';
				WshShellCmd_GetOptValue(&optCtx, argc, pArgv, sizeof(lvlStr),
										(WshShell_Size_t*)&lvlStr);
				switch (lvlStr) {
					case 'T':
					case 't':
						reqLvl = LOG_LVL_TRACE;
						break;
					case 'D':
					case 'd':
						reqLvl = LOG_LVL_DEBUG;
						break;
					case 'I':
					case 'i':
						reqLvl = LOG_LVL_INFO;
						break;
					case 'W':
					case 'w':
						reqLvl = LOG_LVL_WARNING;
						break;
					case 'E':
					case 'e':
						reqLvl = LOG_LVL_ERROR;
						break;
					case 'C':
					case 'c':
						reqLvl = LOG_LVL_CRITIC;
						break;
					default:
						reqLvl = LOG_LVL_WARNING;
						WSH_SHELL_PRINT_WARN("Invalid debug level! Switched on level 'W'\r\n");
						return WSH_SHELL_RET_STATE_ERROR;
				}

				LOG_LVL_t currLvl = Debug_LogLvl_Get();
				if (currLvl == reqLvl) {
					WSH_SHELL_PRINT_INFO("The same lvl selected: ");
				} else {
					Debug_LogLvl_Set(reqLvl);
					WSH_SHELL_PRINT_INFO("New lvl selected: ");
				}

				WSH_SHELL_PRINT_INFO("[%s%s%s]\r\n", Debug_LogLvl_GetColor(reqLvl),
									 Debug_LogLvl_GetStr(reqLvl), ESC_RESET_STYLE);
				break;
			}

			case CMD_DEBUG_LOG_OPT_QUIT:
				Debug_LogLvl_Set(LOG_LVL_DISABLE);
				WSH_SHELL_PRINT_INFO("Debug mode deactivated\r\n");
				break;

			default:
				return WSH_SHELL_RET_STATE_ERROR;
		}
	}

	return WSH_SHELL_RET_STATE_SUCCESS;
}

const WshShellCmd_t Shell_DebugLogCmd = {
	.Groups	 = WSH_SHELL_CMD_GROUP_ADMIN,
	.Name	 = "log",
	.Descr	 = "Enable system log/debug output",
	.Options = DebugOptArr,
	.OptNum	 = CMD_DEBUG_LOG_OPT_ENUM_SIZE,
	.Handler = shell_cmd_debug_log,
};

/* clang-format off */
#define CMD_RESET_OPT_TABLE() \
X_CMD_ENTRY(CMD_RESET_OPT_HELP, WSH_SHELL_OPT_HELP()) \
X_CMD_ENTRY(CMD_RESET_OPT_DEF, WSH_SHELL_OPT_NO(WSH_SHELL_OPT_ACCESS_EXECUTE)) \
X_CMD_ENTRY(CMD_RESET_OPT_HF, WSH_SHELL_OPT_WO_PARAM(WSH_SHELL_OPT_ACCESS_EXECUTE, "-f", "--hard-fault", "Reset with hard fault")) \
X_CMD_ENTRY(CMD_RESET_OPT_END, WSH_SHELL_OPT_END())
/* clang-format on */

#define X_CMD_ENTRY(en, m) en,
typedef enum { CMD_RESET_OPT_TABLE() CMD_RESET_OPT_ENUM_SIZE } CMD_RESET_OPT_t;
#undef X_CMD_ENTRY

#define X_CMD_ENTRY(enum, opt) {enum, opt},
WshShellOption_t ResetOptArr[] = {CMD_RESET_OPT_TABLE()};
#undef X_CMD_ENTRY

static WSH_SHELL_RET_STATE_t shell_cmd_reset(const WshShellCmd_t* pcCmd, WshShell_Size_t argc,
											 const char* pArgv[], void* pCtx) {
	if ((argc > 0 && pArgv == NULL) || pcCmd == NULL)
		return WSH_SHELL_RET_STATE_ERROR;

	WshShell_Size_t tokenPos = 0;
	while (tokenPos < argc) {
		WshShellOption_Context_t optCtx = WshShellCmd_ParseOpt(pcCmd, argc, pArgv, &tokenPos);
		if (optCtx.Option == NULL)
			return WSH_SHELL_RET_STATE_ERR_EMPTY;

		switch (optCtx.Option->ID) {
			case CMD_RESET_OPT_HELP:
				WshShellCmd_PrintOptionsOverview(pcCmd);
				break;

			case CMD_RESET_OPT_DEF:
				Pl_SoftReset();
				break;

			case CMD_RESET_OPT_HF:
				WSH_SHELL_PRINT_INFO("Create HF exception\r\n");
				Debug_CreateHardFault();
				Pl_SoftReset();
				break;

			default:
				return WSH_SHELL_RET_STATE_ERROR;
		}
	}

	return WSH_SHELL_RET_STATE_SUCCESS;
}

const WshShellCmd_t Shell_ResetCmd = {
	.Groups	 = WSH_SHELL_CMD_GROUP_ADMIN,
	.Name	 = "rst",
	.Descr	 = "Reset MCU",
	.Options = ResetOptArr,
	.OptNum	 = CMD_RESET_OPT_ENUM_SIZE,
	.Handler = shell_cmd_reset,
};
