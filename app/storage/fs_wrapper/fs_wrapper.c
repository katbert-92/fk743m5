#include "fs_wrapper.h"
#include "debug.h"
#include "linked_list.h"

#define FS_WRAP_MAX_TIMEOUT portMAX_DELAY

#if DEBUG_ENABLE
#define LOCAL_DEBUG_PRINT_ENABLE 0
#define LOCAL_DEBUG_TEST_ENABLE	 0
#endif /* DEBUG_ENABLE */

#if LOCAL_DEBUG_PRINT_ENABLE
#warning LOCAL_DEBUG_PRINT_ENABLE
#define LOCAL_DEBUG_PRINT DEBUG_LOG_PRINT
#else /* LOCAL_DEBUG_PRINT_ENABLE */
#define LOCAL_DEBUG_PRINT(_f_, ...)
#endif /* LOCAL_DEBUG_PRINT_ENABLE */

#if LOCAL_DEBUG_TEST_ENABLE
#warning LOCAL_DEBUG_TEST_ENABLE
#endif /* LOCAL_DEBUG_TEST_ENABLE */

extern const FsWrap_FileSystem_t FsWrap_Fs_Fatfs;

typedef struct {
	FS_TYPE_t Type;
	const FsWrap_FileSystem_t* pFs;
} FsWrap_RegistryEntry_t;

static FsWrap_RegistryEntry_t Registry[FS_TYPE_ENUM_SIZE];

static LinkedList_Handle_t MountList;

static inline bool FsWrapper_FunctionIsNotImplemented(void* pFunc, char* pFuncName) {
	if (!pFunc) {
		LOCAL_DEBUG_PRINT("%s function is not implemented", pFuncName);
		PANIC();
		return false;
	}

	return true;
}

static inline void FsRegistry_ClearEntry(FsWrap_RegistryEntry_t* pRegEntry) {
	pRegEntry->pFs = NULL;
}

static RET_STATE_t FsRegistry_Add(FS_TYPE_t type, const FsWrap_FileSystem_t* pFs) {
	RET_STATE_t retState = RET_STATE_ERR_OVERFLOW;

	SYS_CRITICAL_ON();
	for (u32 i = 0; i < NUM_ELEMENTS(Registry); ++i) {
		FsWrap_RegistryEntry_t* pRegEntry = &Registry[i];

		if (pRegEntry->pFs == NULL) {
			pRegEntry->Type = type;
			pRegEntry->pFs	= pFs;
			retState		= RET_STATE_SUCCESS;
			break;
		}
	}
	SYS_CRITICAL_OFF();

	return retState;
}

static FsWrap_RegistryEntry_t* FsRegistry_Find(FS_TYPE_t type) {

	SYS_CRITICAL_ON();
	for (u32 i = 0; i < NUM_ELEMENTS(Registry); ++i) {
		FsWrap_RegistryEntry_t* pRegEntry = &Registry[i];

		if ((pRegEntry->pFs != NULL) && (pRegEntry->Type == type)) {
			return pRegEntry;
		}
	}
	SYS_CRITICAL_OFF();

	return NULL;
}

static const FsWrap_FileSystem_t* FsRegistry_GetType(FS_TYPE_t type) {
	FsWrap_RegistryEntry_t* pRegEntry = FsRegistry_Find(type);

	return (pRegEntry != NULL) ? pRegEntry->pFs : NULL;
}

static RET_STATE_t Fs_GetMntPoint(FsWrap_Mount_t** ppMnt, const char* pName, u32* pMatchLen,
								  u32* pNodeId) {

	RET_STATE_t res = LinkedList_ReadWriteLock(&MountList, FS_WRAP_MAX_TIMEOUT, NULL);
	if (res != RET_STATE_SUCCESS) {
		LOCAL_DEBUG_PRINT("Unable to lock MountList");
		return res;
	}

	u32 nameLen			   = strlen(pName);
	u32 longestMatch	   = 0;
	FsWrap_Mount_t* pMntPt = NULL;
	u32 nodes			   = LinkedList_GetNodesNum(&MountList);
	for (u32 i = 0; i < nodes; i++) {
		res = LinkedList_GetDataPtr(&MountList, (void**)&pMntPt, NULL, i);
		if (res != RET_STATE_SUCCESS) {
			return res;
		}

		u32 len = strlen(pMntPt->pMntPointPath);
		/*
		 * Move to next node if mount point length is
		 * shorter than longestMatch match or if path
		 * name is shorter than the mount point name.
		 */
		if ((len < longestMatch) || (len > nameLen)) {
			continue;
		}

		/* Check for mount point match */
		if (strncmp(pName, pMntPt->pMntPointPath, len) == 0) {
			longestMatch = len;
			if (pNodeId) {
				*pNodeId = i;
			}
		}
	}
	LinkedList_ReadWriteUnlock(&MountList);

	if (pMntPt == NULL) {
		return RET_STATE_ERROR;
	}

	*ppMnt = pMntPt;
	if (pMatchLen) {
		*pMatchLen = pMntPt->MntPointSize;
	}

	return RET_STATE_SUCCESS;
}

static inline RET_STATE_t FsWrapper_GetMntPoint(FsWrap_Mount_t** ppMntPt, const char* pPath,
												u32* pMatchLen, u32* pNodeId) {
	RET_STATE_t retState = Fs_GetMntPoint(ppMntPt, pPath, pMatchLen, pNodeId);
	if (retState != RET_STATE_SUCCESS)
		DEBUG_LOG_LVL_PRINT(LOG_LVL_WARNING, "No mount point found by %s", pPath);

	return retState;
}

RET_STATE_t FsWrap_Init(void) {
	RET_STATE_t retState = LinkedList_Create(&MountList, xTaskGetTickCount, vTaskDelay);
	if (retState != RET_STATE_SUCCESS) {
		DEBUG_LOG_LVL_PRINT(LOG_LVL_ERROR, "Unable to create MountList!");
		return retState;
	}

	retState = FsWrap_Register(FS_TYPE_FATFS, &FsWrap_Fs_Fatfs);
	if (retState != RET_STATE_SUCCESS) {
		DEBUG_LOG_LVL_PRINT(LOG_LVL_ERROR, "Unable to register FAT FS");
		return retState;
	}

	return retState;
}

RET_STATE_t FsWrap_Lock(const char* pPath) {

	ASSERT_CHECK(pPath != NULL);

	FsWrap_Mount_t* pMntPt;
	RET_STATE_t getMntPtState = FsWrapper_GetMntPoint(&pMntPt, pPath, NULL, NULL);
	if (getMntPtState != RET_STATE_SUCCESS)
		return getMntPtState;

	if (!FsWrapper_FunctionIsNotImplemented(pMntPt->pFs->lock, "lock"))
		return RET_STATE_ERROR;

	RET_STATE_t retState = pMntPt->pFs->lock(pMntPt);
	if (retState != RET_STATE_SUCCESS) {
		LOCAL_DEBUG_PRINT("Lock failed, %s", RetState_GetStr(retState));
		return retState;
	}

	return retState;
}

RET_STATE_t FsWrap_Unlock(const char* pPath) {

	ASSERT_CHECK(pPath != NULL);

	FsWrap_Mount_t* pMntPt;
	RET_STATE_t getMntPtState = FsWrapper_GetMntPoint(&pMntPt, pPath, NULL, NULL);
	if (getMntPtState != RET_STATE_SUCCESS)
		return getMntPtState;

	if (!FsWrapper_FunctionIsNotImplemented(pMntPt->pFs->unlock, "unlock"))
		return RET_STATE_ERROR;

	RET_STATE_t retState = pMntPt->pFs->unlock(pMntPt);
	if (retState != RET_STATE_SUCCESS) {
		LOCAL_DEBUG_PRINT("Unlock failed, %s", RetState_GetStr(retState));
		return retState;
	}

	return retState;
}

/* File operations */
RET_STATE_t FsWrap_Open(FsWrap_File_t* pFile, const char* pPath, u32 flags) {

	ASSERT_CHECK(pFile != NULL);
	ASSERT_CHECK(pPath != NULL);

	if (pFile == NULL) {
		LOCAL_DEBUG_PRINT("Incorrect file handle");
		return RET_STATE_ERR_PARAM;
	}

	if ((pPath == NULL) || (strlen(pPath) < 1)) {
		LOCAL_DEBUG_PRINT("Incorrect path");
		return RET_STATE_ERR_PARAM;
	}

	if (pFile->pMntPoint != NULL) {
		LOCAL_DEBUG_PRINT("Mount point should be NULL when opening the file");
		return RET_STATE_ERR_PARAM;
	}

	FsWrap_Mount_t* pMntPt;
	RET_STATE_t getMntPtState = FsWrapper_GetMntPoint(&pMntPt, pPath, NULL, NULL);
	if (getMntPtState != RET_STATE_SUCCESS)
		return getMntPtState;

	if (((pMntPt->Flags & FS_MOUNT_FLAG_READ_ONLY) != 0) &&
		(flags & FS_MODE_CREATE_ALWAYS || flags & FS_MODE_CREATE_OR_APPEND ||
		 flags & FS_MODE_WRITE)) {
		LOCAL_DEBUG_PRINT("Can not open file to create/write in read-only FS");
		return RET_STATE_ERROR;
	}

	if (!FsWrapper_FunctionIsNotImplemented(pMntPt->pFs->open, "open"))
		return RET_STATE_ERROR;

	bool truncateFileFlag = false;
	if ((flags & FS_MODE_TRUNCATE) != 0) {
		if ((flags & FS_MODE_WRITE) == 0) {
			LOCAL_DEBUG_PRINT("Truncate not allowed when file is not opened for write");
			return RET_STATE_ERROR;
		}

		if (!FsWrapper_FunctionIsNotImplemented(pMntPt->pFs->truncate, "truncate"))
			return RET_STATE_ERROR;
		truncateFileFlag = true;
	}

	pFile->pMntPoint	 = pMntPt;
	RET_STATE_t retState = pMntPt->pFs->open(pFile, pPath, flags);
	if (retState != RET_STATE_SUCCESS) {
		LOCAL_DEBUG_PRINT("Open error");
		pFile->pMntPoint = NULL;
		return retState;
	}

	/* Copy flags to pFile for use with other fs_ API calls */
	pFile->Flags = flags;

	if (truncateFileFlag) {
		/* Truncate the opened file to 0 length */
		retState = pMntPt->pFs->truncate(pFile, 0);
		if (retState != RET_STATE_SUCCESS) {
			LOCAL_DEBUG_PRINT("Truncate error");
			pFile->pMntPoint = NULL;
			return retState;
		}
	}

	return retState;
}

RET_STATE_t FsWrap_Close(FsWrap_File_t* pFile) {

	ASSERT_CHECK(pFile != NULL);

	if (pFile == NULL) {
		LOCAL_DEBUG_PRINT("Incorrect file handle");
		return RET_STATE_ERR_PARAM;
	}

	if (pFile->pMntPoint == NULL) {
		LOCAL_DEBUG_PRINT("Wrong mount point");
		return RET_STATE_ERR_PARAM;
	}

	if (!FsWrapper_FunctionIsNotImplemented(pFile->pMntPoint->pFs->close, "close"))
		return RET_STATE_ERROR;

	RET_STATE_t retState = pFile->pMntPoint->pFs->close(pFile);
	if (retState != RET_STATE_SUCCESS) {
		LOCAL_DEBUG_PRINT("Unlock failed, %s", RetState_GetStr(retState));
		return retState;
	}

	pFile->pMntPoint = NULL;

	return retState;
}

RET_STATE_t FsWrap_Unlink(const char* pPath) {

	ASSERT_CHECK(pPath != NULL);

	if ((pPath == NULL) || (strlen(pPath) <= 1)) {
		LOCAL_DEBUG_PRINT("Incorrect path");
		return RET_STATE_ERR_PARAM;
	}

	FsWrap_Mount_t* pMntPt;
	RET_STATE_t getMntPtState = FsWrapper_GetMntPoint(&pMntPt, pPath, NULL, NULL);
	if (getMntPtState != RET_STATE_SUCCESS)
		return getMntPtState;

	if (pMntPt->Flags & FS_MOUNT_FLAG_READ_ONLY) {
		LOCAL_DEBUG_PRINT("Can not remove files in read-only FS");
		return RET_STATE_ERROR;
	}

	if (!FsWrapper_FunctionIsNotImplemented(pMntPt->pFs->unlink, "unlink"))
		return RET_STATE_ERROR;

	RET_STATE_t retState = pMntPt->pFs->unlink(pMntPt, pPath);
	if (retState != RET_STATE_SUCCESS) {
		LOCAL_DEBUG_PRINT("Unlink failed, %s", RetState_GetStr(retState));
		return retState;
	}

	return retState;
}

RET_STATE_t FsWrap_Rename(const char* pFrom, const char* pTo) {

	ASSERT_CHECK(pFrom != NULL);
	ASSERT_CHECK(pTo != NULL);

	if ((pFrom == NULL) || (strlen(pFrom) <= 1) || (pTo == NULL) || (strlen(pTo) <= 1)) {
		LOCAL_DEBUG_PRINT("Incorrect path");
		return RET_STATE_ERR_PARAM;
	}

	FsWrap_Mount_t* pMntPt = NULL;
	u32 matchLen;
	RET_STATE_t getMntPtState = FsWrapper_GetMntPoint(&pMntPt, pFrom, &matchLen, NULL);
	if (getMntPtState != RET_STATE_SUCCESS)
		return getMntPtState;

	if (pMntPt->Flags & FS_MOUNT_FLAG_READ_ONLY) {
		LOCAL_DEBUG_PRINT("Can not rename files in read-only FS");
		return RET_STATE_ERROR;
	}

	/* Make sure both files are mounted on the same path */
	if (strncmp(pFrom, pTo, matchLen) != 0) {
		return RET_STATE_ERROR;
	}

	if (!FsWrapper_FunctionIsNotImplemented(pMntPt->pFs->rename, "rename"))
		return RET_STATE_ERROR;

	RET_STATE_t retState = pMntPt->pFs->rename(pMntPt, pFrom, pTo);
	if (retState != RET_STATE_SUCCESS) {
		LOCAL_DEBUG_PRINT("Rename failed, %s", RetState_GetStr(retState));
		return retState;
	}

	return retState;
}

RET_STATE_t FsWrap_Read(FsWrap_File_t* pFile, void* pData, u32 size, u32* pBytesRd) {

	ASSERT_CHECK(pFile != NULL);
	ASSERT_CHECK(pData != NULL);

	if (pFile == NULL) {
		LOCAL_DEBUG_PRINT("Incorrect file handle");
		return RET_STATE_ERR_PARAM;
	}

	if (pFile->pMntPoint == NULL) {
		LOCAL_DEBUG_PRINT("Wrong mount point");
		return RET_STATE_ERR_PARAM;
	}

	if (!FsWrapper_FunctionIsNotImplemented(pFile->pMntPoint->pFs->read, "read"))
		return RET_STATE_ERROR;

	RET_STATE_t retState = pFile->pMntPoint->pFs->read(pFile, pData, size, pBytesRd);
	if (retState != RET_STATE_SUCCESS) {
		LOCAL_DEBUG_PRINT("Read failed, %s", RetState_GetStr(retState));
		return retState;
	}

	return retState;
}

RET_STATE_t FsWrap_Write(FsWrap_File_t* pFile, const void* pData, u32 size, u32* pBytesWr) {

	ASSERT_CHECK(pFile != NULL);
	ASSERT_CHECK(pData != NULL);

	if (pFile == NULL) {
		LOCAL_DEBUG_PRINT("Incorrect file handle");
		return RET_STATE_ERR_PARAM;
	}

	if (pFile->pMntPoint == NULL) {
		LOCAL_DEBUG_PRINT("Wrong mount point");
		return RET_STATE_ERR_PARAM;
	}

	if (!FsWrapper_FunctionIsNotImplemented(pFile->pMntPoint->pFs->write, "write"))
		return RET_STATE_ERROR;

	RET_STATE_t retState = pFile->pMntPoint->pFs->write(pFile, pData, size, pBytesWr);
	if (retState != RET_STATE_SUCCESS) {
		LOCAL_DEBUG_PRINT("Write failed, %s", RetState_GetStr(retState));
		return retState;
	}

	return retState;
}

RET_STATE_t FsWrap_Seek(FsWrap_File_t* pFile, s32 offset, FS_SEEK_t whence) {

	ASSERT_CHECK(pFile != NULL);

	if (pFile == NULL) {
		LOCAL_DEBUG_PRINT("Incorrect file handle");
		return RET_STATE_ERR_PARAM;
	}

	if (pFile->pMntPoint == NULL) {
		LOCAL_DEBUG_PRINT("Wrong mount point");
		return RET_STATE_ERR_PARAM;
	}

	if (!FsWrapper_FunctionIsNotImplemented(pFile->pMntPoint->pFs->lseek, "lseek"))
		return RET_STATE_ERROR;

	RET_STATE_t retState = pFile->pMntPoint->pFs->lseek(pFile, offset, whence);
	if (retState != RET_STATE_SUCCESS) {
		LOCAL_DEBUG_PRINT("Seek failed, %s", RetState_GetStr(retState));
		return retState;
	}

	return retState;
}

RET_STATE_t FsWrap_Tell(FsWrap_File_t* pFile, u32* pPos) {

	ASSERT_CHECK(pFile != NULL);

	if (pFile == NULL) {
		LOCAL_DEBUG_PRINT("Incorrect file handle");
		return RET_STATE_ERR_PARAM;
	}

	if (pFile->pMntPoint == NULL) {
		LOCAL_DEBUG_PRINT("Wrong mount point");
		return RET_STATE_ERR_PARAM;
	}

	if (!FsWrapper_FunctionIsNotImplemented(pFile->pMntPoint->pFs->tell, "tell"))
		return RET_STATE_ERROR;

	RET_STATE_t retState = pFile->pMntPoint->pFs->tell(pFile, pPos);
	if (retState != RET_STATE_SUCCESS) {
		LOCAL_DEBUG_PRINT("Tell failed, %s", RetState_GetStr(retState));
		return retState;
	}

	return retState;
}

RET_STATE_t FsWrap_Size(FsWrap_File_t* pFile, u32* pSize) {

	ASSERT_CHECK(pFile != NULL);

	if (pFile == NULL) {
		LOCAL_DEBUG_PRINT("Incorrect file handle");
		return RET_STATE_ERR_PARAM;
	}

	if (pFile->pMntPoint == NULL) {
		LOCAL_DEBUG_PRINT("Wrong mount point");
		return RET_STATE_ERR_PARAM;
	}

	if (!FsWrapper_FunctionIsNotImplemented(pFile->pMntPoint->pFs->size, "size"))
		return RET_STATE_ERROR;

	RET_STATE_t rc = pFile->pMntPoint->pFs->size(pFile, pSize);
	if (rc != RET_STATE_SUCCESS) {
		LOCAL_DEBUG_PRINT("Size failed, %s", RetState_GetStr(retState));
		return rc;
	}

	return rc;
}

RET_STATE_t FsWrap_Truncate(FsWrap_File_t* pFile, u32 length) {

	ASSERT_CHECK(pFile != NULL);

	if (pFile == NULL) {
		LOCAL_DEBUG_PRINT("Incorrect file handle");
		return RET_STATE_ERR_PARAM;
	}

	if (pFile->pMntPoint == NULL) {
		LOCAL_DEBUG_PRINT("Wrong mount point");
		return RET_STATE_ERR_PARAM;
	}

	if (!FsWrapper_FunctionIsNotImplemented(pFile->pMntPoint->pFs->truncate, "truncate"))
		return RET_STATE_ERROR;

	RET_STATE_t retState = pFile->pMntPoint->pFs->truncate(pFile, length);
	if (retState != RET_STATE_SUCCESS) {
		LOCAL_DEBUG_PRINT("Truncate failed, %s", RetState_GetStr(retState));
		return retState;
	}

	return retState;
}

RET_STATE_t FsWrap_Sync(FsWrap_File_t* pFile) {

	ASSERT_CHECK(pFile != NULL);

	if (pFile == NULL) {
		LOCAL_DEBUG_PRINT("Incorrect file handle");
		return RET_STATE_ERR_PARAM;
	}

	if (pFile->pMntPoint == NULL) {
		LOCAL_DEBUG_PRINT("Wrong mount point");
		return RET_STATE_ERR_PARAM;
	}

	if (!FsWrapper_FunctionIsNotImplemented(pFile->pMntPoint->pFs->sync, "sync"))
		return RET_STATE_ERROR;

	RET_STATE_t retState = pFile->pMntPoint->pFs->sync(pFile);
	if (retState != RET_STATE_SUCCESS) {
		LOCAL_DEBUG_PRINT("Sync failed, %s", RetState_GetStr(retState));
		return retState;
	}

	return retState;
}

RET_STATE_t FsWrap_Forward(FsWrap_File_t* pFile, u32 (*func)(const u8* pBuff, u32 size),
						   u32 bytesNum, u32* pBytesRd) {

	ASSERT_CHECK(pFile != NULL);

	if (pFile == NULL) {
		LOCAL_DEBUG_PRINT("Incorrect file handle");
		return RET_STATE_ERR_PARAM;
	}

	if (pFile->pMntPoint == NULL) {
		LOCAL_DEBUG_PRINT("Wrong mount point");
		return RET_STATE_ERR_PARAM;
	}

	if (pFile->pMntPoint->pFs->forward == NULL) {
		LOCAL_DEBUG_PRINT("Forward function is not implemented");
		return RET_STATE_ERROR;
	}

	RET_STATE_t rc = pFile->pMntPoint->pFs->forward(pFile, func, bytesNum, pBytesRd);
	if (rc != RET_STATE_SUCCESS) {
		LOCAL_DEBUG_PRINT("Forward failed");
		return rc;
	}

	return rc;
}

/* Filesystem operations */
RET_STATE_t FsWrap_Mkdir(const char* pPath) {

	ASSERT_CHECK(pPath != NULL);

	if ((pPath == NULL) || (strlen(pPath) <= 1)) {
		LOCAL_DEBUG_PRINT("Incorrect path");
		return RET_STATE_ERR_PARAM;
	}

	FsWrap_Mount_t* pMntPt;
	RET_STATE_t getMntPtState = FsWrapper_GetMntPoint(&pMntPt, pPath, NULL, NULL);
	if (getMntPtState != RET_STATE_SUCCESS)
		return getMntPtState;

	if (pMntPt->Flags & FS_MOUNT_FLAG_READ_ONLY) {
		LOCAL_DEBUG_PRINT("Can not create directories in read-only FS");
		return RET_STATE_ERROR;
	}

	if (!FsWrapper_FunctionIsNotImplemented(pMntPt->pFs->mkdir, "mkdir"))
		return RET_STATE_ERROR;

	RET_STATE_t retState = pMntPt->pFs->mkdir(pMntPt, pPath);
	if (retState != RET_STATE_SUCCESS) {
		LOCAL_DEBUG_PRINT("Mkdir failed, %s", RetState_GetStr(retState));
		return retState;
	}

	return retState;
}

/* Directory operations */
RET_STATE_t FsWrap_OpenDir(FsWrap_Dir_t* pDir, const char* pPath) {

	ASSERT_CHECK(pDir != NULL);
	ASSERT_CHECK(pPath != NULL);

	if (pDir == NULL) {
		LOCAL_DEBUG_PRINT("Incorrect directory handle");
		return RET_STATE_ERR_PARAM;
	}

	if ((pPath == NULL) || (strlen(pPath) < 1)) {
		LOCAL_DEBUG_PRINT("Incorrect path");
		return RET_STATE_ERR_PARAM;
	}

	if (pDir->pMntPoint != NULL || pDir->pDirHandler != NULL) {
		LOCAL_DEBUG_PRINT("pDir object should be empty");
		return RET_STATE_ERR_PARAM;
	}

	FsWrap_Mount_t* pMntPt;
	RET_STATE_t getMntPtState = FsWrapper_GetMntPoint(&pMntPt, pPath, NULL, NULL);
	if (getMntPtState != RET_STATE_SUCCESS)
		return getMntPtState;

	if (!FsWrapper_FunctionIsNotImplemented(pMntPt->pFs->opendir, "opendir"))
		return RET_STATE_ERROR;

	pDir->pMntPoint = pMntPt;

	RET_STATE_t retState = pDir->pMntPoint->pFs->opendir(pDir, pPath);
	if (retState != RET_STATE_SUCCESS) {
		LOCAL_DEBUG_PRINT("Opendir failed, %s", RetState_GetStr(retState));
		pDir->pMntPoint	  = NULL;
		pDir->pDirHandler = NULL;
		return retState;
	}

	return retState;
}

RET_STATE_t FsWrap_ReadDir(FsWrap_Dir_t* pDir, FsWrap_DirEnt_t* pEntry) {

	ASSERT_CHECK(pDir != NULL);
	ASSERT_CHECK(pEntry != NULL);

	if (pDir == NULL) {
		LOCAL_DEBUG_PRINT("Incorrect directory handle");
		return RET_STATE_ERR_PARAM;
	}

	if (!pDir->pMntPoint) {
		LOCAL_DEBUG_PRINT("Wrong mount point");
		return RET_STATE_ERR_PARAM;
	}

	if (!FsWrapper_FunctionIsNotImplemented(pDir->pMntPoint->pFs->readdir, "readdir"))
		return RET_STATE_ERROR;

	RET_STATE_t retState = RET_STATE_ERROR;
	/* Loop until error or not special directory */
	while (true) {
		retState = pDir->pMntPoint->pFs->readdir(pDir, pEntry);
		if (retState != RET_STATE_SUCCESS) {
			LOCAL_DEBUG_PRINT("Readdir failed, %s", RetState_GetStr(retState));
			break;
		}
		if (pEntry->Name[0] == 0) {
			break;
		}
		if (pEntry->Type != FS_DIR_ENTRY_DIR) {
			break;
		}
		if ((strcmp(pEntry->Name, ".") != 0) && (strcmp(pEntry->Name, "..") != 0)) {
			break;
		}
	}

	return retState;
}

RET_STATE_t FsWrap_CloseDir(FsWrap_Dir_t* pDir) {

	ASSERT_CHECK(pDir != NULL);

	if (pDir == NULL) {
		LOCAL_DEBUG_PRINT("Incorrect directory handle");
		return RET_STATE_ERR_PARAM;
	}

	if (pDir->pMntPoint == NULL) {
		pDir->pDirHandler = NULL;
		LOCAL_DEBUG_PRINT("Directory hasn't been opened");
		return RET_STATE_SUCCESS;
	}

	if (!FsWrapper_FunctionIsNotImplemented(pDir->pMntPoint->pFs->closedir, "closedir"))
		return RET_STATE_ERROR;

	RET_STATE_t retState = pDir->pMntPoint->pFs->closedir(pDir);
	if (retState != RET_STATE_SUCCESS) {
		LOCAL_DEBUG_PRINT("Closedir failed, %s", RetState_GetStr(retState));
		return retState;
	}

	pDir->pMntPoint	  = NULL;
	pDir->pDirHandler = NULL;
	return retState;
}

RET_STATE_t FsWrap_Stat(const char* pPath, FsWrap_DirEnt_t* pEntry) {

	ASSERT_CHECK(pPath != NULL);
	ASSERT_CHECK(pEntry != NULL);

	if ((pPath == NULL) || (strlen(pPath) <= 1)) {
		LOCAL_DEBUG_PRINT("Incorrect path");
		return RET_STATE_ERR_PARAM;
	}

	FsWrap_Mount_t* pMntPt;
	RET_STATE_t getMntPtState = FsWrapper_GetMntPoint(&pMntPt, pPath, NULL, NULL);
	if (getMntPtState != RET_STATE_SUCCESS)
		return getMntPtState;

	if (!FsWrapper_FunctionIsNotImplemented(pMntPt->pFs->stat, "stat"))
		return RET_STATE_ERROR;

	RET_STATE_t retState = pMntPt->pFs->stat(pMntPt, pPath, pEntry);
	if (retState != RET_STATE_SUCCESS) {
		LOCAL_DEBUG_PRINT("Stat failed, %s", RetState_GetStr(retState));
		return retState;
	}

	return retState;
}

RET_STATE_t FsWrap_StatVFS(const char* pPath, FsWrap_StatVfs_t* pStat) {

	ASSERT_CHECK(pPath != NULL);
	ASSERT_CHECK(pStat != NULL);

	if ((pPath == NULL) || (strlen(pPath) <= 1)) {
		LOCAL_DEBUG_PRINT("Incorrect path");
		return RET_STATE_ERR_PARAM;
	}

	FsWrap_Mount_t* pMntPt;
	RET_STATE_t getMntPtState = FsWrapper_GetMntPoint(&pMntPt, pPath, NULL, NULL);
	if (getMntPtState != RET_STATE_SUCCESS)
		return getMntPtState;

	if (!FsWrapper_FunctionIsNotImplemented(pMntPt->pFs->statvfs, "statvfs"))
		return RET_STATE_ERROR;

	RET_STATE_t retState = pMntPt->pFs->statvfs(pMntPt, pPath, pStat);
	if (retState != RET_STATE_SUCCESS) {
		LOCAL_DEBUG_PRINT("StatVFS failed, %s", RetState_GetStr(retState));
		return retState;
	}

	return retState;
}

RET_STATE_t FsWrap_SetLabel(const char* pLabel) {

	ASSERT_CHECK(pLabel != NULL);

	if ((pLabel == NULL) || (strlen(pLabel) <= 1)) {
		LOCAL_DEBUG_PRINT("Incorrect label");
		return RET_STATE_ERR_PARAM;
	}

	FsWrap_Mount_t* pMntPt;
	RET_STATE_t getMntPtState = FsWrapper_GetMntPoint(&pMntPt, pLabel, NULL, NULL);
	if (getMntPtState != RET_STATE_SUCCESS)
		return getMntPtState;

	if (!FsWrapper_FunctionIsNotImplemented(pMntPt->pFs->setlabel, "setlabel"))
		return RET_STATE_ERROR;

	RET_STATE_t retState = pMntPt->pFs->setlabel(pLabel);
	if (retState != RET_STATE_SUCCESS) {
		LOCAL_DEBUG_PRINT("SetLabel failed, %s", RetState_GetStr(retState));
		return retState;
	}

	return retState;
}

RET_STATE_t FsWrap_GetLabel(const char* pPath, char* pLabel, u32* pVsn) {
	ASSERT_CHECK(pPath != NULL);
	ASSERT_CHECK(pLabel != NULL);

	if ((pPath == NULL) || (strlen(pPath) <= 1) || (pLabel == NULL)) {
		LOCAL_DEBUG_PRINT("Incorrect path or label");
		return RET_STATE_ERR_PARAM;
	}

	FsWrap_Mount_t* pMntPt;
	RET_STATE_t getMntPtState = FsWrapper_GetMntPoint(&pMntPt, pPath, NULL, NULL);
	if (getMntPtState != RET_STATE_SUCCESS)
		return getMntPtState;

	if (!FsWrapper_FunctionIsNotImplemented(pMntPt->pFs->getlabel, "getlabel"))
		return RET_STATE_ERROR;

	RET_STATE_t retState = pMntPt->pFs->getlabel(pPath, pLabel, pVsn);
	if (retState != RET_STATE_SUCCESS) {
		LOCAL_DEBUG_PRINT("GetLabel failed, %s", RetState_GetStr(retState));
		return retState;
	}

	return retState;
}

#if FS_FF_USE_MKFS

RET_STATE_t FsWrap_Mkfs(FS_TYPE_t fsType, char* pDevId, void* pCfg, u32 flags) {

	ASSERT_CHECK(pDevId != NULL);

	/* Get file system information */
	const FsWrap_FileSystem_t* pFs = FsRegistry_GetType(fsType);
	if (pFs == NULL) {
		DEBUG_LOG_LVL_PRINT(LOG_LVL_WARNING, "No FS registered with type %d", (u32)fsType);
		return RET_STATE_ERROR;
	}

	if (!FsWrapper_FunctionIsNotImplemented(pFs->mkfs, "mkfs"))
		return RET_STATE_ERROR;

	RET_STATE_t retState = pFs->mkfs(pDevId, pCfg, flags);
	if (retState != RET_STATE_SUCCESS) {
		DEBUG_LOG_LVL_PRINT(LOG_LVL_WARNING, "Mkfs failed, %s", RetState_GetStr(retState));
		return retState;
	}

	return retState;
}

#endif /* FS_FF_USE_MKFS */

RET_STATE_t FsWrap_Mount(FsWrap_Mount_t* pMntPoint) {

	ASSERT_CHECK(pMntPoint != NULL);

	/* Do all the mp checks prior to locking the mutex on the file
	 * subsystem.
	 */
	if ((pMntPoint == NULL) || (pMntPoint->pMntPointPath == NULL)) {
		DEBUG_LOG_LVL_PRINT(LOG_LVL_WARNING, "Mount point should be empty when mounting the FS");
		return RET_STATE_ERR_PARAM;
	}

	u32 len = strlen(pMntPoint->pMntPointPath);

	if (len <= 1) {
		DEBUG_LOG_LVL_PRINT(LOG_LVL_WARNING, "Mount point path is incorrect");
		return RET_STATE_ERR_PARAM;
	}

	/* Check if mount point already exists */
	RET_STATE_t retState = LinkedList_ReadWriteLock(&MountList, FS_WRAP_MAX_TIMEOUT, NULL);
	if (retState != RET_STATE_SUCCESS) {
		DEBUG_LOG_LVL_PRINT(LOG_LVL_WARNING, "Unable to lock MountList");
		return retState;
	}

	FsWrap_Mount_t* pMntPt = NULL;
	u32 nodesNum		   = LinkedList_GetNodesNum(&MountList);
	for (u32 i = 0; i < nodesNum; i++) {
		retState = LinkedList_GetDataPtr(&MountList, (void**)&pMntPt, NULL, i);

		if (retState != RET_STATE_SUCCESS) {
			DEBUG_LOG_LVL_PRINT(LOG_LVL_WARNING, "Can't access MountList item");
			return retState;
		}

		len = strlen(pMntPt->pMntPointPath);

		/* continue if length does not match */
		if (len != pMntPt->MntPointSize) {
			continue;
		}

		if (pMntPoint->pFsData == pMntPt->pFsData) {
			DEBUG_LOG_LVL_PRINT(LOG_LVL_INFO, "FS on this path is already mounted");
			LinkedList_ReadWriteUnlock(&MountList);
			return RET_STATE_ERROR;
		}

		if (strncmp(pMntPoint->pMntPointPath, pMntPt->pMntPointPath, len) == 0) {
			DEBUG_LOG_LVL_PRINT(LOG_LVL_INFO, "FS on this path is already mounted");
			LinkedList_ReadWriteUnlock(&MountList);
			return RET_STATE_ERROR;
		}
	}
	LinkedList_ReadWriteUnlock(&MountList);

	/* Get file system information */
	const FsWrap_FileSystem_t* pFs = FsRegistry_GetType(pMntPoint->Type);
	if (pFs == NULL) {
		DEBUG_LOG_LVL_PRINT(LOG_LVL_WARNING, "Incorrect FS path");
		return RET_STATE_ERROR;
	}

	if (!FsWrapper_FunctionIsNotImplemented(pFs->mount, "mount"))
		return RET_STATE_ERROR;

	retState = pFs->mount(pMntPoint);
	if (retState != RET_STATE_SUCCESS) {
		DEBUG_LOG_LVL_PRINT(LOG_LVL_WARNING, "Mount failed, %s", RetState_GetStr(retState));
		return RET_STATE_ERROR;
	}

	/* Update mount point data and append it to the list */
	pMntPoint->MntPointSize = len;
	pMntPoint->pFs			= pFs;

	return LinkedList_Insert(&MountList, pMntPoint, sizeof(FsWrap_Mount_t), 0);
}

RET_STATE_t FsWrap_Unmount(FsWrap_Mount_t* pMntPoint) {

	ASSERT_CHECK(pMntPoint != NULL);

	if (pMntPoint == NULL) {
		LOCAL_DEBUG_PRINT("Mount point is NULL");
		return RET_STATE_ERR_PARAM;
	}

	FsWrap_Mount_t* pMntPt = NULL;
	u32 node			   = 0;
	RET_STATE_t retState   = Fs_GetMntPoint(&pMntPt, pMntPoint->pMntPointPath, NULL, &node);

	if (retState != RET_STATE_SUCCESS) {
		if (pMntPt == NULL) {
			LOCAL_DEBUG_PRINT("Can not find FS on requested mount point");
			return retState;
		}

		LOCAL_DEBUG_PRINT("Undefined error looking for FS on requested mount point");
		return retState;
	}

	if (!FsWrapper_FunctionIsNotImplemented(pMntPoint->pFs->unmount, "unmount"))
		return RET_STATE_ERROR;

	retState = pMntPoint->pFs->unmount(pMntPoint);
	if (retState != RET_STATE_SUCCESS) {
		LOCAL_DEBUG_PRINT("Unmount failed, %s", RetState_GetStr(retState));
		return RET_STATE_ERROR;
	}

	/* clear file system interface */
	pMntPt->pFs = NULL;

	/* remove mount node from the list */
	LinkedList_Extract(&MountList, NULL, NULL, node);

	return retState;
}

/* Register File system */
RET_STATE_t FsWrap_Register(u32 type, const FsWrap_FileSystem_t* pFs) {

	ASSERT_CHECK(pFs != NULL);

	if (FsRegistry_GetType(type) != NULL) {
		DEBUG_LOG_LVL_PRINT(LOG_LVL_WARNING, "Wrong FS type");
		return RET_STATE_ERROR;
	}

	RET_STATE_t retState = FsRegistry_Add(type, pFs);
	if (retState != RET_STATE_SUCCESS) {
		DEBUG_LOG_LVL_PRINT(LOG_LVL_WARNING, "FS register failed, %s", RetState_GetStr(retState));
	}

	return retState;
}

/* Unregister File system */
RET_STATE_t FsWrap_Unregister(u32 type, const FsWrap_FileSystem_t* pFs) {

	ASSERT_CHECK(pFs != NULL);

	FsWrap_RegistryEntry_t* pRegEntry;

	pRegEntry = FsRegistry_Find(type);
	if ((pRegEntry == NULL) || (pRegEntry->pFs != pFs)) {
		LOCAL_DEBUG_PRINT("Wrong FS type");
		return RET_STATE_ERROR;
	}

	FsRegistry_ClearEntry(pRegEntry);

	return RET_STATE_SUCCESS;
}
