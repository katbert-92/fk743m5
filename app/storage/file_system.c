#include "file_system.h"
#include "debug.h"
#include "fs_wrapper.h"
#include "mem_wrapper.h"
#include "storage.h"
#include "storage_utils.h"
#include "stringlib.h"
#include "time_date.h"
#include "wsh_shell.h"

#if DEBUG_ENABLE
#define LOCAL_DEBUG_PRINT_ENABLE 0
#define LOCAL_DEBUG_TEST_ENABLE	 0
#endif /* DEBUG_ENABLE */

#if LOCAL_DEBUG_PRINT_ENABLE
#warning LOCAL_DEBUG_PRINT_ENABLE
#define LOCAL_DEBUG_PRINT	  DEBUG_PRINT_NL
#define LOCAL_DEBUG_LOG_PRINT DEBUG_LOG_PRINT
#else /* LOCAL_DEBUG_PRINT_ENABLE */
#define LOCAL_DEBUG_PRINT(_f_, ...)
#define LOCAL_DEBUG_LOG_PRINT(_f_, ...)
#endif /* LOCAL_DEBUG_PRINT_ENABLE */

#define FS_SHELL_BUFF_SIZE		 512
#define FS_SHELL_MINUTES_TTL_DEF 3
#define FS_SHELL_MINUTES_TTL_MAX 5

static STORAGE_DRIVE_t Storage_CurrDrive = STORAGE_DRIVE_EMMC;
static char Storage_CurrDrivePath[4];

static RET_STATE_t FileSystem_ScanFiles(const char* pPath, const char* pPrefix) {
	RET_STATE_t res = RET_STATE_SUCCESS;

	char* pCurrPath =
		(char*)MemWrap_Malloc(FS_MAX_FILE_NAME + 1, __FILENAME__, __LINE__, DELAY_1_SECOND * 5);
	char* pNewPefix =
		MemWrap_Malloc(FS_MAX_FILE_NAME + 1, __FILENAME__, __LINE__, DELAY_1_SECOND * 5);
	FsWrap_Dir_t* pDir =
		MemWrap_Malloc(sizeof(FsWrap_Dir_t), __FILENAME__, __LINE__, DELAY_1_SECOND * 5);
	FsWrap_DirEnt_t* pFno =
		MemWrap_Malloc(sizeof(FsWrap_DirEnt_t), __FILENAME__, __LINE__, DELAY_1_SECOND * 5);
	if (!pCurrPath || !pNewPefix || !pDir || !pFno) {
		WSH_SHELL_PRINT("Memory allocation failed for path!\r\n");
		return RET_STATE_ERR_MEMORY;
	}

	strncpy(pCurrPath, pPath, FS_MAX_FILE_NAME);
	memset(pDir, 0, sizeof(FsWrap_Dir_t));

	res = FsWrap_OpenDir(pDir, pCurrPath);
	if (res != RET_STATE_SUCCESS) {
		WSH_SHELL_PRINT("Failed to open directory: %s\r\n", pCurrPath);
		MemWrap_Free(pCurrPath);
		MemWrap_Free(pDir);
		MemWrap_Free(pFno);
		return res;
	}

	u32 dirCnt = 0;
	while (FsWrap_ReadDir(pDir, pFno) == RET_STATE_SUCCESS) {
		if (pFno->Name[0] == 0) {
			FsWrap_CloseDir(pDir);
			break;
		}

		dirCnt++;
	}

	res = FsWrap_OpenDir(pDir, pCurrPath);
	if (res != RET_STATE_SUCCESS) {
		WSH_SHELL_PRINT_ERR("Failed to open directory: %s\r\n", pCurrPath);
		MemWrap_Free(pCurrPath);
		MemWrap_Free(pDir);
		MemWrap_Free(pFno);
		return res;
	}

	u32 currCnt = 0;
	while (FsWrap_ReadDir(pDir, pFno) == RET_STATE_SUCCESS) {
		if (pFno->Name[0] == 0)
			break;

		currCnt++;

		// Skip "." and ".."
		if (pFno->Name[0] == '.')
			continue;

		bool isLast = currCnt == dirCnt;

		u32 pCurPathLen = strnlen(pCurrPath, FS_MAX_FILE_NAME);
		if (pCurPathLen + strlen(pFno->Name) > FS_MAX_FILE_NAME) {
			WSH_SHELL_PRINT_WARN("Path too long: %s/%s\r\n", pCurrPath, pFno->Name);
			continue;
		}

		WSH_SHELL_PRINT("%s%s── %s\r\n", pPrefix, isLast ? "└" : "├", pFno->Name);

		if (pFno->Type & FS_DIR_ENTRY_DIR) {
			snprintf(&pCurrPath[pCurPathLen], FS_MAX_FILE_NAME - pCurPathLen, "/%s", pFno->Name);
			snprintf(pNewPefix, FS_MAX_FILE_NAME + 1, "%s%s   ", pPrefix, isLast ? " " : "│");

			res = FileSystem_ScanFiles(pCurrPath, pNewPefix);
			if (res != RET_STATE_SUCCESS) {
				break;
			}
		}

		pCurrPath[pCurPathLen] = 0;
	}

	FsWrap_CloseDir(pDir);
	MemWrap_Free(pCurrPath);
	MemWrap_Free(pNewPefix);
	MemWrap_Free(pDir);
	MemWrap_Free(pFno);

	return res;
}

/* clang-format off */
#define CMD_FS_OPT_TABLE() \
X_ENTRY(CMD_FS_OPT_HELP, WSH_SHELL_OPT_HELP()) \
X_ENTRY(CMD_FS_OPT_DEF, WSH_SHELL_OPT_NO(WSH_SHELL_OPT_ACCESS_READ)) \
X_ENTRY(CMD_FS_OPT_DISK_NUM, WSH_SHELL_OPT_INT(WSH_SHELL_OPT_ACCESS_EXECUTE, "-d", "--disc", "Set disk root path number")) \
X_ENTRY(CMD_FS_OPT_SCAN, WSH_SHELL_OPT_WO_PARAM(WSH_SHELL_OPT_ACCESS_READ, "-s", "--scan", "Scan files")) \
X_ENTRY(CMD_FS_OPT_MSD, WSH_SHELL_OPT_INT(WSH_SHELL_OPT_ACCESS_EXECUTE, "-m", "--msd", "Run MSD for specified amount of minutes")) \
X_ENTRY(CMD_FS_OPT_INFO, WSH_SHELL_OPT_WO_PARAM(WSH_SHELL_OPT_ACCESS_READ, "-i", "--info", "Show information about storage")) \
X_ENTRY(CMD_FS_OPT_TEST_SPEED, WSH_SHELL_OPT_WO_PARAM(WSH_SHELL_OPT_ACCESS_READ, "-t", "--testspeed", "Fast speed test")) \
X_ENTRY(CMD_FS_OPT_END, WSH_SHELL_OPT_END())
/* clang-format on */

#define X_ENTRY(en, m) en,
typedef enum { CMD_FS_OPT_TABLE() CMD_FS_OPT_ENUM_SIZE } CMD_FS_OPT_t;
#undef X_ENTRY

#define X_ENTRY(enum, opt) {enum, opt},
WshShellOption_t FsOptArr[] = {CMD_FS_OPT_TABLE()};
#undef X_ENTRY

static WSH_SHELL_RET_STATE_t shell_cmd_fs(const WshShellCmd_t* pcCmd, WshShell_Size_t argc,
										  const char* pArgv[], void* pCtx) {
	if ((argc > 0 && pArgv == NULL) || pcCmd == NULL)
		return WSH_SHELL_RET_STATE_ERROR;

	u32 n								 = 0;
	char infoBuff[FS_SHELL_BUFF_SIZE]	 = "";
	char prettyPrint[FS_SHELL_BUFF_SIZE] = "";
	u32 ttlMin							 = FS_SHELL_MINUTES_TTL_DEF;
	snprintf(Storage_CurrDrivePath, sizeof(Storage_CurrDrivePath), "%d:", Storage_CurrDrive);

	WshShell_Size_t tokenPos = 0;
	while (tokenPos < argc) {
		WshShellOption_Context_t optCtx = WshShellCmd_ParseOpt(pcCmd, argc, pArgv, &tokenPos);
		if (optCtx.Option == NULL)
			return WSH_SHELL_RET_STATE_ERR_EMPTY;

		switch (optCtx.Option->ID) {
			case CMD_FS_OPT_HELP:
			case CMD_FS_OPT_DEF:
				WshShellCmd_PrintOptionsOverview(pcCmd);
				break;

			case CMD_FS_OPT_DISK_NUM: {
				WshShellCmd_GetOptValue(&optCtx, argc, pArgv, sizeof(Storage_CurrDrive),
										(WshShell_Size_t*)&Storage_CurrDrive);

				if (Storage_CurrDrive >= STORAGE_DRIVE_ENUM_SIZE) {
					WSH_SHELL_PRINT_WARN("Drive '%d:' unavailable\r\n", Storage_CurrDrive);
					Storage_CurrDrive = STORAGE_DRIVE_EMMC;
					WSH_SHELL_PRINT_INFO("Switched to drive '%d:'\r\n", Storage_CurrDrive);
				}
				snprintf(Storage_CurrDrivePath, sizeof(Storage_CurrDrivePath),
						 "%d:", Storage_CurrDrive);
				WSH_SHELL_PRINT_INFO("Drive '%s' selected\r\n", Storage_CurrDrivePath);
				break;
			}

			case CMD_FS_OPT_SCAN:
				char localPath[FS_MAX_FILE_NAME + 1];
				strcpy(localPath, Storage_CurrDrivePath);
				FileSystem_ScanFiles(localPath, "");
				break;

			case CMD_FS_OPT_MSD: {
				WshShellCmd_GetOptValue(&optCtx, argc, pArgv, sizeof(Storage_CurrDrive),
										(WshShell_Size_t*)&ttlMin);
				if (ttlMin > FS_SHELL_MINUTES_TTL_MAX)
					ttlMin = FS_SHELL_MINUTES_TTL_MAX;
				else if (ttlMin == 0)
					ttlMin = FS_SHELL_MINUTES_TTL_DEF;

				if (Pl_USB_IsClassMSC()) {
					WSH_SHELL_PRINT_WARN("MSD storage already plugged!\r\n");
					return WSH_SHELL_RET_STATE_WARNING;
				}

				if (FsWrap_Lock(Storage_CurrDrivePath) == RET_STATE_ERR_TIMEOUT) {
					WSH_SHELL_PRINT_WARN("File system is busy!\r\n");
				}

				WSH_SHELL_PRINT("MSD storage plugged\r\n");
				Pl_USB_MSC_Init();

				// u32 tsEnd = xTaskGetTickCount() + ttlMin * DELAY_1_MINUTE;
				// while (true) {
				// 	if (Touch_GetState() == TOUCH_STATE_PRESS) {
				// 		do {
				// 			vTaskDelay(250);
				// 			WSH_SHELL_PRINT("*");
				// 		} while (Touch_GetState() == TOUCH_STATE_PRESS);
				// 		break;
				// 	}

				// 	if (xTaskGetTickCount() > tsEnd) {
				// 		tsEnd = Storage_GetReadWriteOps_LastTime() + 10 * DELAY_1_SECOND;
				// 		if (PL_GET_MS_CNT() > tsEnd) {
				// 			break;
				// 		}
				// 	}

				// 	vTaskDelay(250);
				// }

				// Pl_USB_CDC_Init(Shell_RxCallbackUSB, Shell_GetReceiveBuff(), 1);
				// Shell_Hardware_Init();
				// TODO switch to uart
				Pl_USB_DeInit();
				FsWrap_Unlock(Storage_CurrDrivePath);
				WSH_SHELL_PRINT("\r\nMSD storage unplugged\r\n");
				break;
			}

			case CMD_FS_OPT_INFO: {
				RET_STATE_t retState = RET_STATE_SUCCESS;

				n += sprintf(infoBuff + n, JSON_FIELD_FIRST, "cmd", pcCmd->Name);
				if (Storage_CurrDrive == STORAGE_DRIVE_EMMC) {
					n += sprintf(infoBuff + n, JSON_FIELD_STR_STR, "emmcHwIsInit",
								 JSON_BOOL_VAL_GET(Storage_EmmcHw_IsInit()));
					n += sprintf(infoBuff + n, JSON_FIELD_STR_STR, "emmcFsIsInit",
								 JSON_BOOL_VAL_GET(Storage_EmmcFs_IsInit()));

					if (Storage_EmmcHw_IsInit()) {
						Pl_SdEmmcInfo_t cardInfo = Storage_GetEmmcInfo();
						n += sprintf(infoBuff + n, JSON_FIELD_STR_ULONG, "mfgID", cardInfo.Class);
						n += sprintf(infoBuff + n, JSON_FIELD_STR_STR, "name", cardInfo.ProdName);
						n += sprintf(infoBuff + n, JSON_FIELD_STR_ULONG, "rev", cardInfo.ProdRev);
						n += sprintf(infoBuff + n, JSON_FIELD_STR_ULONG, "SN", cardInfo.ProdSN);
						n += sprintf(infoBuff + n, JSON_FIELD_STR_ULONG, "cardType",
									 cardInfo.CardType);
						n += sprintf(infoBuff + n, JSON_FIELD_STR_ULONG, "class", cardInfo.Class);
						n += sprintf(infoBuff + n, JSON_FIELD_STR_ULONG, "relCardAdd",
									 cardInfo.RelCardAdd);
						n += sprintf(infoBuff + n, JSON_FIELD_STR_ULONG, "blockNbr",
									 cardInfo.BlockNbr);
						n += sprintf(infoBuff + n, JSON_FIELD_STR_ULONG, "blockSize",
									 cardInfo.BlockSize);
						n += sprintf(infoBuff + n, JSON_FIELD_STR_ULONG, "logBlockNbr",
									 cardInfo.LogBlockNbr);
						n += sprintf(infoBuff + n, JSON_FIELD_STR_ULONG, "logBlockSize",
									 cardInfo.LogBlockSize);
					} else {
						retState = RET_STATE_ERROR;
					}

				} else if (Storage_CurrDrive == STORAGE_DRIVE_RAM) {
					n += sprintf(infoBuff + n, JSON_FIELD_STR_STR, "ramHwIsInit",
								 JSON_BOOL_VAL_GET(Storage_RamHw_IsInit()));
					n += sprintf(infoBuff + n, JSON_FIELD_STR_STR, "ramFsIsInit",
								 JSON_BOOL_VAL_GET(Storage_RamFs_IsInit()));
				} else {
					retState = RET_STATE_ERROR;
				}

				n += sprintf(infoBuff + n, JSON_FIELD_STR_STR, "result", RetState_GetStr(retState));
				n += sprintf(infoBuff + n, JSON_FIELD_LAST, JSON_KEY_TSTAMP,
							 TimeDate_Timestamp_Get());
				STRING_LIB_JSON_PRETTY_PRINT_DEF(infoBuff, prettyPrint, FS_SHELL_BUFF_SIZE);
				WSH_SHELL_PRINT(prettyPrint);

				break;
			}

			case CMD_FS_OPT_TEST_SPEED: {
				float rSpeed, wSpeed;
				RET_STATE_t retState =
					StorageUtils_FsSpeedTest(Storage_CurrDrivePath, &rSpeed, &wSpeed);

				n += sprintf(infoBuff + n, JSON_FIELD_FIRST, "cmd", pcCmd->Name);
				n += sprintf(infoBuff + n, JSON_FIELD_STR_STR, "drive", Storage_CurrDrivePath);
				n += sprintf(infoBuff + n, JSON_FIELD_STR_FLT, "speedWrite", wSpeed);
				n += sprintf(infoBuff + n, JSON_FIELD_STR_FLT, "speedRead", rSpeed);
				n += sprintf(infoBuff + n, JSON_FIELD_STR_STR, "result", RetState_GetStr(retState));
				n += sprintf(infoBuff + n, JSON_FIELD_LAST, JSON_KEY_TSTAMP,
							 TimeDate_Timestamp_Get());
				STRING_LIB_JSON_PRETTY_PRINT_DEF(infoBuff, prettyPrint, FS_SHELL_BUFF_SIZE);

				WSH_SHELL_PRINT(prettyPrint);
				break;
			}

			default:
				return WSH_SHELL_RET_STATE_ERROR;
		}
	}

	return WSH_SHELL_RET_STATE_SUCCESS;
}

const WshShellCmd_t Shell_FileSystemCmd = {
	.Groups	 = WSH_SHELL_CMD_GROUP_ADMIN,
	.Name	 = "fs",
	.Descr	 = "File system actions",
	.Options = FsOptArr,
	.OptNum	 = CMD_FS_OPT_ENUM_SIZE,
	.Handler = shell_cmd_fs,
};
