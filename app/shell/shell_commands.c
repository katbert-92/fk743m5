#include "shell_commands.h"

// extern const WshShellCmd_t Shell_RtosCmd;
extern const WshShellCmd_t Shell_FileSystemCmd;
extern const WshShellCmd_t Shell_DebugLogCmd;
extern const WshShellCmd_t Shell_ResetCmd;

static const WshShellCmd_t* Shell_CmdTable[] = {
	// &Shell_RtosCmd,
	&Shell_FileSystemCmd,
	&Shell_DebugLogCmd,
	&Shell_ResetCmd,
};

bool Shell_Commands_Init(WshShell_t* pShell) {
	return WshShellRetState_TranslateToProject(
		WshShellCmd_Attach(&(pShell->Commands), Shell_CmdTable, NUM_ELEMENTS(Shell_CmdTable)));
}
