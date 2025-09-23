#include "shell_users.h"
#include "stringlib.h"
#include "wsh_shell_user.h"

static const WshShellUser_t Shell_UserTable[] = {
	{
		.Login	= "admin",
		.Salt	= "538a03bccc40a07f",
		.Hash	= "06bcec27",  //1234
		.Groups = WSH_SHELL_CMD_GROUP_ADMIN,
		.Rights = WSH_SHELL_USER_ACCESS_ADMIN,
	},
};

RET_STATE_t Shell_Users_Init(WshShell_t* pShell) {
	return WshShellRetState_TranslateToProject(WshShellUser_Attach(
		&(pShell->Users), Shell_UserTable, NUM_ELEMENTS(Shell_UserTable), NULL));
}
