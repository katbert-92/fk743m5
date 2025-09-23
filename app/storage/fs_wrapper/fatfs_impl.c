#include "ff.h"
#include "fs_wrapper.h"
#include "main.h"
#include "mem_wrapper.h"
#include "stringlib.h"

static RET_STATE_t TranslateFsRetCode(FRESULT error) {
	switch (error) {
		case FR_OK:
			return RET_STATE_SUCCESS;
		case FR_NOT_ENOUGH_CORE:
			return RET_STATE_ERR_MEMORY;
		case FR_INVALID_PARAMETER:
			return RET_STATE_ERR_PARAM;
		case FR_NO_FILE:
			return RET_STATE_ERR_EMPTY;
		case FR_EXIST:
			return RET_STATE_ERR_BUSY;
		default:
			return RET_STATE_ERROR;
	}
}

static u32 TranslateFlags(u32 flags) {
	u32 ffMode = 0;

	ffMode |= (flags & FS_MODE_READ) ? FA_READ : 0;
	ffMode |= (flags & FS_MODE_WRITE) ? FA_WRITE : 0;
	ffMode |= (flags & FS_MODE_CREATE_ALWAYS) ? FA_CREATE_ALWAYS : 0;
	ffMode |= (flags & FS_MODE_CREATE_OR_APPEND) ? FA_OPEN_ALWAYS : 0;
	/* NOTE: FA_APPEND is not translated because FAT FS does not
	 * support append semantics of the Zephyr, where file position
	 * is forwarded to the end before each write, the fatfs_write
	 * will be tasked with setting a file position to the end,
	 * if FA_APPEND flag is present.
	 */

	return ffMode;
}

static RET_STATE_t FatFS_Open(FsWrap_File_t* pFile, const char* pPath, u32 flags) {
	pFile->pFileHandler = MemWrap_Malloc(sizeof(FIL), __FILE__, __LINE__, MEM_ALLOC_UNLIM_TMO);
	(void)memset(pFile->pFileHandler, 0x00, sizeof(FIL));

	u32 ffMode	= TranslateFlags(flags);
	FRESULT res = f_open(pFile->pFileHandler, pPath, (u8)ffMode);

	if (res != FR_OK) {
		MemWrap_Free(pFile->pFileHandler);
		pFile->pFileHandler = NULL;
	}

	return TranslateFsRetCode(res);
}

static RET_STATE_t FatFS_Close(FsWrap_File_t* pFile) {
	FRESULT res = f_close(pFile->pFileHandler);
	MemWrap_Free(pFile->pFileHandler);
	pFile->pFileHandler = NULL;

	return TranslateFsRetCode(res);
}

static RET_STATE_t FatFS_Unlink(FsWrap_Mount_t* pMntPoint, const char* pPath) {
#if !FF_FS_READONLY
	FRESULT res = f_unlink(pPath);
	return TranslateFsRetCode(res);
#endif

	return RET_STATE_ERROR;
}

static RET_STATE_t FatFS_Rename(FsWrap_Mount_t* pMntPoint, const char* pFrom, const char* pTo) {
#if !FF_FS_READONLY
	FILINFO fno;

	/* Check if 'to' path exists; remove it if it does */
	FRESULT res = f_stat(pTo, &fno);
	if (res == FR_OK) {
		res = f_unlink(pTo);
		if (res != FR_OK) {
			return TranslateFsRetCode(res);
		}
	}

	res = f_rename(pFrom, pTo);
	return TranslateFsRetCode(res);
#endif

	return RET_STATE_ERROR;
}

static RET_STATE_t FatFS_Read(FsWrap_File_t* pFile, void* pDest, u32 bytesNum, u32* pBytesRd) {
	FRESULT res = f_read(pFile->pFileHandler, pDest, bytesNum, (size_t*)pBytesRd);
	return TranslateFsRetCode(res);
}

static RET_STATE_t FatFS_Write(FsWrap_File_t* pFile, const void* pSrc, u32 bytesNum,
							   u32* pBytesWr) {
#if !FF_FS_READONLY
	u32 pos		= f_size((FIL*)pFile->pFileHandler);
	FRESULT res = FR_OK;

	/* FA_APPEND flag means that file has been opened for append.
	 * The FAT FS write does not support the POSIX append semantics,
	 * to always write at the end of file, so set file position
	 * at the end before each write if FA_APPEND is set.
	 */
	if ((pFile->Flags & FS_MODE_APPEND) || (pFile->Flags & FS_MODE_CREATE_OR_APPEND)) {
		res = f_lseek(pFile->pFileHandler, pos);
	}

	if (res == FR_OK) {
		res = f_write(pFile->pFileHandler, pSrc, bytesNum, (size_t*)pBytesWr);
	}

	return TranslateFsRetCode(res);
#endif

	return RET_STATE_ERROR;
}

static RET_STATE_t FatFS_Seek(FsWrap_File_t* pFile, s32 offset, FS_SEEK_t whence) {
	FRESULT res = FR_OK;
	u32 pos;

	switch (whence) {
		case FS_SEEK_START:
			pos = offset;
			break;
		case FS_SEEK_CUR:
			pos = f_tell((FIL*)pFile->pFileHandler) + offset;
			break;
		case FS_SEEK_END:
			pos = f_size((FIL*)pFile->pFileHandler) + offset;
			break;
		default:
			return RET_STATE_ERROR;
	}

	if ((pos < 0) || (pos > f_size((FIL*)pFile->pFileHandler))) {
		return RET_STATE_ERROR;
	}

	res = f_lseek(pFile->pFileHandler, pos);
	return TranslateFsRetCode(res);
}

static RET_STATE_t FatFS_Tell(FsWrap_File_t* pFile, u32* pPos) {
	*pPos = f_tell((FIL*)pFile->pFileHandler);

	if (*pPos >= 0) {
		return RET_STATE_SUCCESS;
	} else {
		return RET_STATE_ERROR;
	}
}

static RET_STATE_t FatFS_Size(FsWrap_File_t* pFile, u32* pSize) {
	*pSize = f_size((FIL*)pFile->pFileHandler);

	if (*pSize >= 0) {
		return RET_STATE_SUCCESS;
	} else {
		return RET_STATE_ERROR;
	}
}

static RET_STATE_t FatFS_Truncate(FsWrap_File_t* pFile, u32 length) {
#if !FF_FS_READONLY
	u32 curLength = f_size((FIL*)pFile->pFileHandler);

	/* f_lseek expands file if new position is larger than file size */
	FRESULT res = f_lseek(pFile->pFileHandler, length);
	if (res != FR_OK) {
		return TranslateFsRetCode(res);
	}

	if (length < curLength) {
		res = f_truncate(pFile->pFileHandler);
	} else {
		/*
		 * Get actual length after expansion. This could be
		 * less if there was not enough space in the volume
		 * to expand to the requested length
		 */
		length = f_tell((FIL*)pFile->pFileHandler);

		res = f_lseek(pFile->pFileHandler, curLength);
		if (res != FR_OK) {
			return TranslateFsRetCode(res);
		}

		/*
		 * The FS module does caching and optimization of
		 * writes. Here we write 1 byte at a time to avoid
		 * using additional code and memory for doing any
		 * optimization.
		 */
		size_t bw;
		u8 c = 0;

		for (u32 i = curLength; i < length; i++) {
			res = f_write(pFile->pFileHandler, &c, 1, &bw);
			if (res != FR_OK) {
				break;
			}
		}
	}

	return TranslateFsRetCode(res);
#endif

	return RET_STATE_ERROR;
}

static RET_STATE_t FatFS_Sync(FsWrap_File_t* pFile) {
#if !FF_FS_READONLY
	FRESULT res = f_sync(pFile->pFileHandler);
	return TranslateFsRetCode(res);
#endif

	return RET_STATE_ERROR;
}

static RET_STATE_t FatFS_Forward(FsWrap_File_t* pFile, u32 (*func)(const u8* pBuff, u32 size),
								 u32 bytesNum, u32* pBytesRd) {
#if FF_USE_FORWARD
	FRESULT res = f_forward(pFile->pFileHandler, (UINT (*)(const BYTE*, UINT))func, (UINT)bytesNum,
							(UINT*)pBytesRd);
	return TranslateFsRetCode(res);
#endif

	return RET_STATE_ERROR;
}

static RET_STATE_t FatFS_Mkdir(FsWrap_Mount_t* pMntPoint, const char* pPath) {
#if !FF_FS_READONLY
	FRESULT res = f_mkdir(pPath);
	return TranslateFsRetCode(res);
#endif

	return RET_STATE_ERROR;
}

static RET_STATE_t FatFS_OpenDir(FsWrap_Dir_t* pDir, const char* pPath) {
	FRESULT res;

	pDir->pDirHandler = MemWrap_Malloc(sizeof(DIR), __FILENAME__, __LINE__, MEM_ALLOC_UNLIM_TMO);
	memset(pDir->pDirHandler, 0, sizeof(DIR));

	res = f_opendir(pDir->pDirHandler, pPath);

	if (res != FR_OK) {
		MemWrap_Free(pDir->pDirHandler);
		pDir->pDirHandler = NULL;
	}

	return TranslateFsRetCode(res);
}

static RET_STATE_t FatFS_ReadDir(FsWrap_Dir_t* pDir, FsWrap_DirEnt_t* pEntry) {
	FRESULT res;
	FILINFO fno;

	res = f_readdir(pDir->pDirHandler, &fno);
	if (res == FR_OK) {
		strcpy(pEntry->Name, fno.fname);
		if (pEntry->Name[0] != 0) {
			pEntry->Type = ((fno.fattrib & AM_DIR) ? FS_DIR_ENTRY_DIR : FS_DIR_ENTRY_FILE);
			pEntry->Size = fno.fsize;
		}
	}

	return TranslateFsRetCode(res);
}

static RET_STATE_t FatFS_CloseDir(FsWrap_Dir_t* pDir) {
	FRESULT res;

	res = f_closedir(pDir->pDirHandler);

	MemWrap_Free(pDir->pDirHandler);
	pDir->pDirHandler = NULL;

	return TranslateFsRetCode(res);
}

static RET_STATE_t FatFS_Stat(FsWrap_Mount_t* pMntPoint, const char* pPath,
							  FsWrap_DirEnt_t* pEntry) {
	FRESULT res;
	FILINFO fno;

	res = f_stat(pPath, &fno);
	if (res == FR_OK) {
		pEntry->Type = ((fno.fattrib & AM_DIR) ? FS_DIR_ENTRY_DIR : FS_DIR_ENTRY_FILE);
		strcpy(pEntry->Name, fno.fname);
		pEntry->Size = fno.fsize;
	}

	return TranslateFsRetCode(res);
}

static RET_STATE_t FatFS_StatVFS(FsWrap_Mount_t* pMntPoint, const char* pPath,
								 FsWrap_StatVfs_t* pStat) {
	RET_STATE_t res = RET_STATE_ERROR;

#if !defined(CONFIG_FS_FATFS_READ_ONLY)
	FATFS* fs;
	DWORD blockFree = 0;

	FRESULT fres = f_getfree(pMntPoint->pMntPointPath, &blockFree, &fs);
	if (fres != FR_OK) {
		return RET_STATE_ERROR;
	}

	pStat->FreeBlocksNum = blockFree;

	/**
	 * If FF_MIN_SS and FF_MAX_SS differ, variable sector size support is
	 * enabled and the file system object structure contains the actual sector
	 * size, otherwise it is configured to a fixed value give by FF_MIN_SS.
	 */
#if FF_MAX_SS != FF_MIN_SS
	stat->TransferBlockSize = fs->ssize;
#else
	pStat->TransferBlockSize = FF_MIN_SS;
#endif
	pStat->AllocUnitSize  = fs->csize * pStat->TransferBlockSize;
	pStat->TotalBlocksNum = (fs->n_fatent - 2);

	res = TranslateFsRetCode(fres);
#endif

	return res;
}

static RET_STATE_t FatFS_Mount(FsWrap_Mount_t* pMntPoint) {
	FRESULT res;

	res = f_mount((FATFS*)pMntPoint->pFsData, pMntPoint->pMntPointPath, 1);

#if FS_FF_USE_MKFS
	if (res == FR_NO_FILESYSTEM && (pMntPoint->Flags & FS_MOUNT_FLAG_READ_ONLY) != 0) {
		return RET_STATE_ERROR;
	}
	/* If no file system found then create one */
	if (res == FR_NO_FILESYSTEM && (pMntPoint->Flags & FS_MOUNT_FLAG_NO_FORMAT) == 0) {
		u8 work[FF_MAX_SS];
		MKFS_PARM mkfs_opt = {
			.fmt	 = FM_ANY | FM_SFD, /* Any suitable FAT */
			.n_fat	 = 1,				/* One FAT fs table */
			.align	 = 0,				/* Get sector size via diskio query */
			.n_root	 = FS_FATFS_MAX_ROOT_ENTRIES,
			.au_size = 0 /* Auto calculate cluster size */
		};

		res = f_mkfs(pMntPoint->pMntPointPath, &mkfs_opt, work, sizeof(work));
		if (res == FR_OK) {
			res = f_mount((FATFS*)pMntPoint->pFsData, pMntPoint->pMntPointPath, 1);
		}
	}
#endif /* FS_FF_USE_MKFS */

	if (res == FR_OK) {
		pMntPoint->Flags |= FS_MOUNT_FLAG_USE_DISK_ACCESS;
	}

	return TranslateFsRetCode(res);
}

static RET_STATE_t FatFS_Unmount(FsWrap_Mount_t* pMntPoint) {
	FRESULT res;
	//DRESULT disk_res;
	//u8 param = DISK_IOCTL_POWER_OFF;

	res = f_mount(NULL, pMntPoint->pMntPointPath, 0);
	if (res != FR_OK) {
		return TranslateFsRetCode(res);
	}

	// /* Make direct disk IOCTL call to deinit disk */
	// disk_res = disk_ioctl(((FATFS *)mountp->fs_data)->pdrv, CTRL_POWER, &param);
	// if (disk_res != RES_OK) {
	// 	LOG_ERR("Could not power off disk (%d)", disk_res);
	// 	return translate_disk_error(disk_res);
	// }

	return TranslateFsRetCode(res);
}

#if FS_FF_USE_MKFS

static MKFS_PARM defCfg = {
	.fmt	 = FM_ANY | FM_SFD, /* Any suitable FAT */
	.n_fat	 = 1,				/* One FAT fs table */
	.align	 = 0,				/* Get sector size via diskio query */
	.n_root	 = FS_FATFS_MAX_ROOT_ENTRIES,
	.au_size = 0 /* Auto calculate cluster size */
};

static RET_STATE_t FatFS_Mkfs(char* devId, void* pCfg, u32 flags) {
	FRESULT res;
	u8 work[FF_MAX_SS];
	MKFS_PARM* pMkfsOpt = &defCfg;

	if (pCfg != NULL) {
		pMkfsOpt = (MKFS_PARM*)pCfg;
	}

	res = f_mkfs((char*)devId, pMkfsOpt, work, sizeof(work));

	return TranslateFsRetCode(res);
}

#endif /* FS_FF_USE_MKFS */

#if FF_USE_LABEL

static RET_STATE_t FatFS_SetLabel(const char* pLabel) {
#if !FF_FS_READONLY
	FRESULT res = f_setlabel(pLabel);
	return TranslateFsRetCode(res);
#endif
	return RET_STATE_ERROR;
}

static RET_STATE_t FatFS_GetLabel(const char* pPath, char* pLabel, u32* pVsn) {
	FRESULT res = f_getlabel(pPath, pLabel, pVsn);

	return TranslateFsRetCode(res);
}

#endif /* FF_USE_LABEL */

static RET_STATE_t FatFS_Lock(FsWrap_Mount_t* pMntPoint) {
	RET_STATE_t res;

	// Take all the volume mutexes
	for (u32 i = 0; i < FF_VOLUMES; i++) {
		res = ff_mutex_take(i);
		if (res != 1) {
			return RET_STATE_ERR_TIMEOUT;
		}
	}

	// And the system one
	res = ff_mutex_take(FF_VOLUMES);

	return res == 1 ? RET_STATE_SUCCESS : RET_STATE_ERR_TIMEOUT;
}

static RET_STATE_t FatFS_Unlock(FsWrap_Mount_t* pMntPoint) {
	// Take all the volume mutexes
	for (u32 i = 0; i < FF_VOLUMES; i++) {
		ff_mutex_give(i);
	}

	// And the system one
	ff_mutex_give(FF_VOLUMES);
	return RET_STATE_SUCCESS;
}

/* File system interface */
const FsWrap_FileSystem_t FsWrap_Fs_Fatfs = {
	.open	  = FatFS_Open,
	.close	  = FatFS_Close,
	.read	  = FatFS_Read,
	.write	  = FatFS_Write,
	.lseek	  = FatFS_Seek,
	.tell	  = FatFS_Tell,
	.size	  = FatFS_Size,
	.truncate = FatFS_Truncate,
	.sync	  = FatFS_Sync,
	.forward  = FatFS_Forward,
	.opendir  = FatFS_OpenDir,
	.readdir  = FatFS_ReadDir,
	.closedir = FatFS_CloseDir,
	.mount	  = FatFS_Mount,
	.unmount  = FatFS_Unmount,
	.unlink	  = FatFS_Unlink,
	.rename	  = FatFS_Rename,
	.mkdir	  = FatFS_Mkdir,
	.stat	  = FatFS_Stat,
	.statvfs  = FatFS_StatVFS,
	.setlabel = FatFS_SetLabel,
	.getlabel = FatFS_GetLabel,
	.lock	  = FatFS_Lock,
	.unlock	  = FatFS_Unlock,
#if FS_FF_USE_MKFS
	.mkfs = FatFS_Mkfs,
#endif
};
