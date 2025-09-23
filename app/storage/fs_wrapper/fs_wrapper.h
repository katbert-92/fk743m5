#ifndef __FS_WRAPPER_H
#define __FS_WRAPPER_H

#include "main.h"

#define FS_FATFS_MAX_ROOT_ENTRIES 512
#define FS_MAX_FILE_NAME		  256

#define FS_FF_USE_MKFS 1

#define FS_MOUNT_FLAG_NO_FORMAT		  0b00000001
#define FS_MOUNT_FLAG_READ_ONLY		  0b00000010
#define FS_MOUNT_FLAG_AUTOMOUNT		  0b00000100
#define FS_MOUNT_FLAG_USE_DISK_ACCESS 0b00001000

/* Opens the file for reading. */
#define FS_MODE_READ			 0b00000001
/* Opens the file for writing. */
#define FS_MODE_WRITE			 0b00000010
/* Creates a new file. If the file already exists, it is truncated and overwritten. */
#define FS_MODE_CREATE_ALWAYS	 0b00010000
/* Opens the file if it exists. If the file does not exist, a new file is created. */
#define FS_MODE_CREATE_OR_APPEND 0b00100000
/* Auxiliary flag used to set the file pointer to the end of the file before each write */
#define FS_MODE_APPEND			 0b01000000
/* Auxiliary flag used to remove all the content from the file and set the file pointer to the start position. */
#define FS_MODE_TRUNCATE		 0b10000000

typedef enum {
	FS_TYPE_FATFS = 0,
	FS_TYPE_LITTLEFS,
	FS_TYPE_EXTERNAL_BASE,

	FS_TYPE_ENUM_SIZE
} FS_TYPE_t;

typedef enum {
	FS_SEEK_START,
	FS_SEEK_CUR,
	FS_SEEK_END,
} FS_SEEK_t;

typedef enum {
	FS_DIR_ENTRY_FILE = 0,
	FS_DIR_ENTRY_DIR,
} FS_DIR_ENTRY_TYPE_t;

typedef struct fs_file_system_s FsWrap_FileSystem_t;

typedef struct {
	FS_TYPE_t Type;
	const char* pMntPointPath;
	void* pFsData;
	/* The following fields are filled by file system core */
	u32 MntPointSize;
	const FsWrap_FileSystem_t* pFs;
	u32 Flags;
} FsWrap_Mount_t;

typedef struct {
	void* pFileHandler;
	const FsWrap_Mount_t* pMntPoint;
	u32 Flags;
} FsWrap_File_t;

typedef struct {
	void* pDirHandler;
	const FsWrap_Mount_t* pMntPoint;
} FsWrap_Dir_t;

typedef struct {
	FS_DIR_ENTRY_TYPE_t Type;
	char Name[FS_MAX_FILE_NAME + 1];
	u32 Size;
} FsWrap_DirEnt_t;

typedef struct {
	/** Optimal transfer block size */
	u32 TransferBlockSize;
	/** Allocation unit size */
	u32 AllocUnitSize;
	/** Size of FS in f_frsize units */
	u32 TotalBlocksNum;
	/** Number of free blocks */
	u32 FreeBlocksNum;
} FsWrap_StatVfs_t;

struct fs_file_system_s {
	/**
	 * @name File operations
	 * @{
	 */
	RET_STATE_t (*open)(FsWrap_File_t* pFile, const char* pPath, u32 flags);
	RET_STATE_t (*read)(FsWrap_File_t* pFile, void* pDest, u32 bytesNum, u32* pBytesRd);
	RET_STATE_t (*write)(FsWrap_File_t* pFile, const void* pSrc, u32 bytesNum, u32* pBytesWr);
	RET_STATE_t (*lseek)(FsWrap_File_t* pFile, s32 offset, FS_SEEK_t whence);
	RET_STATE_t (*tell)(FsWrap_File_t* pFile, u32* pPos);
	RET_STATE_t (*size)(FsWrap_File_t* pFile, u32* pSize);
	RET_STATE_t (*truncate)(FsWrap_File_t* pFile, u32 length);
	RET_STATE_t (*sync)(FsWrap_File_t* pFile);
	RET_STATE_t (*forward)(FsWrap_File_t* pFile, u32 (*func)(const u8* pBuff, u32 size),
						   u32 bytesNum, u32* pBytesRd);
	RET_STATE_t (*close)(FsWrap_File_t* pFile);
	/** @} */

	/**
	 * @name Directory operations
	 * @{
	 */
	RET_STATE_t (*opendir)(FsWrap_Dir_t* pDir, const char* pPath);
	RET_STATE_t (*readdir)(FsWrap_Dir_t* pDir, FsWrap_DirEnt_t* pEntry);
	RET_STATE_t (*closedir)(FsWrap_Dir_t* pDir);
	/** @} */

	/**
	 * @name File system level operations
	 * @{
	 */
	RET_STATE_t (*mount)(FsWrap_Mount_t* pMntPoint);
	RET_STATE_t (*unmount)(FsWrap_Mount_t* pMntPoint);
	RET_STATE_t (*unlink)(FsWrap_Mount_t* pMntPoint, const char* pPath);
	RET_STATE_t (*rename)(FsWrap_Mount_t* pMntPoint, const char* pFrom, const char* pTo);
	RET_STATE_t (*mkdir)(FsWrap_Mount_t* pMntPoint, const char* pPath);
	RET_STATE_t (*stat)(FsWrap_Mount_t* pMntPoint, const char* pPath, FsWrap_DirEnt_t* pEntry);
	RET_STATE_t (*statvfs)(FsWrap_Mount_t* pMntPoint, const char* pPath, FsWrap_StatVfs_t* pStat);
	RET_STATE_t (*setlabel)(const char* pLabel);
	RET_STATE_t (*getlabel)(const char* pPath, char* pLabel, u32* pVsn);

	RET_STATE_t (*lock)(FsWrap_Mount_t* pMntPoint);
	RET_STATE_t (*unlock)(FsWrap_Mount_t* pMntPoint);

#if FS_FF_USE_MKFS
	RET_STATE_t (*mkfs)(char* devId, void* pCfg, u32 flags);
#endif

	/** @} */
};

/**
 * @brief Initialize a File System Wrapper
 *
 * Initializes all internal tools to maintain the wrapper and registers 
 * FatFS implementation.
 *
 * @retval RET_STATE_SUCCESS on success;
 * @retval RET_STATE_ERROR on any internal error.
 */
RET_STATE_t FsWrap_Init(void);

/**
 * @brief Lock a volume for both read and write operations
 * 
 * This function should only be used when some external mechanism intends to 
 * access selected drive bypassing the regular flow. In any other cases wrapper 
 * function usage is self-sufficient and do not require this one to be called.
 *
 * @param pPath Path to a mounted volume to lock
 * 
 * @retval RET_STATE_SUCCESS on success;
 * @retval RET_STATE_ERROR on any internal error.
 */
RET_STATE_t FsWrap_Lock(const char* pPath);

/**
 * @brief Unlock a volume
 * 
 * This function should only be used when some external mechanism fisished 
 * accessing selected drive bypassing the regular flow. In any other cases 
 * wrapper function usage is self-sufficient and do not require this one to 
 * be called.
 * 
 * @param pPath Path to a mounted volume to unlock
 *
 * @retval RET_STATE_SUCCESS on success;
 * @retval RET_STATE_ERROR on any internal error.
 */
RET_STATE_t FsWrap_Unlock(const char* pPath);

/**
 * @brief Open or create file
 *
 * Opens or possibly creates a file and associates a stream with it.
 * Successfully opened file, when no longer in use, should be closed
 * with fs_close().
 *
 * @details
 * @p flags can be 0 or a binary combination of one or more of the following
 * identifiers:
 *   - @c FS_MODE_READ open for read
 *   - @c FS_MODE_WRITE open for write
 *   - @c FS_MODE_CREATE create file if it does not exist
 *   - @c FS_MODE_APPEND move to end of file before each write
 *   - @c FS_MODE_TRUNC truncate the file
 *
 * @warning If @p flags are set to 0 the function will open file, if it exists
 *          and is accessible, but you will have no read/write access to it.
 *
 * @param pFile Pointer to a file object
 * @param pPath The name of a file to open
 * @param flags The mode flags
 *
 * @retval RET_STATE_SUCCESS on success;
 * @retval RET_STATE_ERR_PARAM when a bad file name is given;
 * @retval RET_STATE_ERROR or an other errno code, depending on a file system back-end.
 */
RET_STATE_t FsWrap_Open(FsWrap_File_t* pFile, const char* pPath, u32 flags);

/**
 * @brief Close file
 *
 * Flushes the associated stream and closes the file.
 *
 * @param pFile Pointer to the file object
 *
 * @retval RET_STATE_SUCCESS on success;
 * @retval RET_STATE_ERR_PARAM when a bad file name is given;
 * @retval RET_STATE_ERROR or an other errno code, depending on a file system back-end.
 */
RET_STATE_t FsWrap_Close(FsWrap_File_t* pFile);

/**
 * @brief Unlink file
 *
 * Deletes the specified file or directory
 *
 * @param pPath Path to the file or directory to delete
 *
 * @retval RET_STATE_SUCCESS on success;
 * @retval RET_STATE_ERR_PARAM when a bad file name is given;
 * @retval RET_STATE_ERROR or an other errno code, depending on a file system back-end.
 */
RET_STATE_t FsWrap_Unlink(const char* pPath);

/**
 * @brief Rename file or directory
 *
 * Performs a rename and / or move of the specified source path to the
 * specified destination.  The source path can refer to either a file or a
 * directory.  All intermediate directories in the destination path must
 * already exist.  If the source path refers to a file, the destination path
 * must contain a full filename path, rather than just the new parent
 * directory.  If an object already exists at the specified destination path,
 * this function causes it to be unlinked prior to the rename (i.e., the
 * destination gets clobbered).
 * @note Current implementation does not allow moving files between mount
 * points.
 *
 * @param pFrom The source path
 * @param pTo The destination path
 *
 * @retval RET_STATE_SUCCESS on success;
 * @retval RET_STATE_ERR_PARAM when a bad file name is given;
 * @retval RET_STATE_ERROR or an other errno code, depending on a file system back-end.
 */
RET_STATE_t FsWrap_Rename(const char* pFrom, const char* pTo);

/**
 * @brief Read file
 *
 * Reads up to @p size bytes of data to @p pData pointed buffer, returns number
 * of bytes read.  A returned value may be lower than @p size if there were
 * fewer bytes available than requested.
 *
 * @param pFile Pointer to the file object
 * @param pData Pointer to the data buffer
 * @param size Number of bytes to be read
 * @param pBytesRd Pointer to a number of bytes actually read
 *
 * @retval RET_STATE_SUCCESS on success;
 * @retval RET_STATE_ERR_PARAM when invoked on pFile that represents unopened/closed file;
 * @retval RET_STATE_ERROR or an other errno code, depending on a file system back-end.
 */
RET_STATE_t FsWrap_Read(FsWrap_File_t* pFile, void* pData, u32 size, u32* pBytesRd);

/**
 * @brief Write file
 *
 * Attempts to write @p size number of bytes to the specified file.
 * If a negative value is returned from the function, the file pointer has not
 * been advanced.
 * If the function returns a non-negative number that is lower than @p size,
 * the global @c errno variable should be checked for an error code,
 * as the device may have no free space for data.
 *
 * @param pFile Pointer to the file object
 * @param pData Pointer to the data buffer
 * @param size Number of bytes to be written
 * @param pBytesWr Pointer to a number of bytes actually read
 *
 * @retval RET_STATE_SUCCESS on success;
 * @retval RET_STATE_ERR_PARAM when invoked on pFile that represents unopened/closed file;
 * @retval RET_STATE_ERROR or an other errno code, depending on a file system back-end.
 */
RET_STATE_t FsWrap_Write(FsWrap_File_t* pFile, const void* pData, u32 size, u32* pBytesWr);

/**
 * @brief Seek file
 *
 * Moves the file position to a new location in the file. The @p offset is added
 * to file position based on the @p whence parameter.
 *
 * @param pFile Pointer to the file object
 * @param offset Relative location to move the file pointer to
 * @param whence Relative location from where offset is to be calculated.
 * - @c FS_SEEK_SET for the beginning of the file;
 * - @c FS_SEEK_CUR for the current position;
 * - @c FS_SEEK_END for the end of the file.
 *
 * @retval RET_STATE_SUCCESS on success;
 * @retval RET_STATE_ERR_PARAM when invoked on pFile that represents unopened/closed file;
 * @retval RET_STATE_ERROR or an other errno code, depending on a file system back-end.
 */
RET_STATE_t FsWrap_Seek(FsWrap_File_t* pFile, s32 offset, FS_SEEK_t whence);

/**
 * @brief Get current file position.
 *
 * Retrieves and returns the current position in the file stream.
 *
 * @param pFile Pointer to the file object
 * @param pPos Pointer to the current file position
 *
 * @retval RET_STATE_SUCCESS on success;
 * @retval RET_STATE_ERR_PARAM when invoked on pFile that represents unopened/closed file;
 * @retval RET_STATE_ERROR or an other errno code, depending on a file system back-end.
 *
 * The current revision does not validate the file object.
 */
RET_STATE_t FsWrap_Tell(FsWrap_File_t* pFile, u32* pPos);

/**
 * @brief Get file size.
 *
 * Retrieves and returns the current size of the file.
 *
 * @param pFile Pointer to the file object
 * @param pSize Size of the file
 *
 * @retval RET_STATE_SUCCESS on success;
 * @retval RET_STATE_ERR_PARAM when invoked on pFile that represents unopened/closed file;
 * @retval RET_STATE_ERROR or an other errno code, depending on a file system back-end.
 */
RET_STATE_t FsWrap_Size(FsWrap_File_t* pFile, u32* pSize);

/**
 * @brief Truncate or extend an open file to a given size
 *
 * Truncates the file to the new length if it is shorter than the current
 * size of the file. Expands the file if the new length is greater than the
 * current size of the file. The expanded region would be filled with zeroes.
 *
 * @note In the case of expansion, if the volume got full during the
 * expansion process, the function will expand to the maximum possible length
 * and return success.  Caller should check if the expanded size matches the
 * requested length.
 *
 * @param pFile Pointer to the file object
 * @param length New size of the file in bytes
 *
 * @retval RET_STATE_SUCCESS on success;
 * @retval RET_STATE_ERR_PARAM when invoked on pFile that represents unopened/closed file;
 * @retval RET_STATE_ERROR or an other errno code, depending on a file system back-end.
 */
RET_STATE_t FsWrap_Truncate(FsWrap_File_t* pFile, u32 length);

/**
 * @brief Flush cached write data buffers of an open file
 *
 * The function flushes the cache of an open file; it can be invoked to ensure
 * data gets written to the storage media immediately, e.g. to avoid data loss
 * in case if power is removed unexpectedly.
 * @note Closing a file will cause caches to be flushed correctly so the
 * function need not be called when the file is being closed.
 *
 * @param pFile Pointer to the file object
 *
 * @retval RET_STATE_SUCCESS on success;
 * @retval RET_STATE_ERR_PARAM when invoked on pFile that represents unopened/closed file;
 * @retval RET_STATE_ERROR or an other errno code, depending on a file system back-end.
 */
RET_STATE_t FsWrap_Sync(FsWrap_File_t* pFile);

/**
 * @brief The function streaming file read
 *
 * @param pFile Pointer to the file object
 * @param func Pointer to callback function
 * @param bytesNum Number of bytes to read
 * @param pBytesRead Number of actually read bytes
 *
 * @retval RET_STATE_SUCCESS on success;
 * @retval RET_STATE_ERR_PARAM when invoked on pFile that represents unopened/closed file;
 * @retval RET_STATE_ERROR or an other errno code, depending on a file system back-end.
 */
RET_STATE_t FsWrap_Forward(FsWrap_File_t* pFile, u32 (*func)(const u8* pBuff, u32 size),
						   u32 bytesNum, u32* pBytesRd);

/**
 * @brief Directory create
 *
 * Creates a new directory using specified path.
 *
 * @param pPath Path to the directory to create
 *
 * @retval RET_STATE_SUCCESS on success;
 * @retval RET_STATE_ERR_PARAM when a bad file name is given or if entry of given name exists;
 * @retval RET_STATE_ERROR or an other errno code, depending on a file system back-end.
 */
RET_STATE_t FsWrap_Mkdir(const char* pPath);

/**
 * @brief Directory open
 *
 * Opens an existing directory specified by the path.
 *
 * @param pDir Pointer to the directory object
 * @param pPath Path to the directory to open
 *
 * @retval RET_STATE_SUCCESS on success;
 * @retval RET_STATE_ERR_PARAM when a bad directory path is given;
 * @retval RET_STATE_ERROR or an other errno code, depending on a file system back-end.
 */
RET_STATE_t FsWrap_OpenDir(FsWrap_Dir_t* pDir, const char* pPath);

/**
 * @brief Directory read entry
 *
 * Reads directory entries of an open directory. In end-of-dir condition,
 * the function will return 0 and set the <tt>entry->name[0]</tt> to 0.
 *
 * @note: Most existing underlying file systems do not generate POSIX
 * special directory entries "." or "..".  For consistency the
 * abstraction layer will remove these from lower layer results so
 * higher layers see consistent results.
 *
 * @param pDir Pointer to the directory object
 * @param pEntry Pointer to zfs_dirent structure to read the entry into
 *
 * @retval RET_STATE_SUCCESS on success;
 * @retval RET_STATE_ERR_PARAM when no such directory found;
 * @retval RET_STATE_ERROR or an other errno code, depending on a file system back-end.
 */
RET_STATE_t FsWrap_ReadDir(FsWrap_Dir_t* pDir, FsWrap_DirEnt_t* pEntry);

/**
 * @brief Directory close
 *
 * Closes an open directory.
 *
 * @param pDir Pointer to the directory object
 *
 * @retval RET_STATE_SUCCESS on success;
 * @retval RET_STATE_ERR_PARAM when a bad directory path is given;
 * @retval RET_STATE_ERROR or an other errno code, depending on a file system back-end.
 */
RET_STATE_t FsWrap_CloseDir(FsWrap_Dir_t* pDir);

/**
 * @brief Mount filesystem
 *
 * Perform steps needed for mounting a file system like
 * calling the file system specific mount function and adding
 * the mount point to mounted file system list.
 *
 * @note Current implementation of ELM FAT driver allows only following mount 
 * points that consist of single digit, e.g: "/0:", "/1:" and so forth.
 *
 * @param pMntPoint Pointer to the FsWrap_Mount_t structure.  Referenced object
 *	     is not changed if the mount operation failed.
 *	     A reference is captured in the fs infrastructure if the
 *	     mount operation succeeds, and the application must not
 *	     mutate the structure contents until fs_unmount is
 *	     successfully invoked on the same pointer.
 *
 * @retval RET_STATE_SUCCESS on success;
 * @retval RET_STATE_ERR_PARAM when a bad input parameter is given;
 * @retval RET_STATE_ERROR or an other errno code, depending on a file system back-end.
 */
RET_STATE_t FsWrap_Mount(FsWrap_Mount_t* pMntPoint);

/**
 * @brief Unmount filesystem
 *
 * Perform steps needed to unmount a file system like
 * calling the file system specific unmount function and removing
 * the mount point from mounted file system list.
 *
 * @param pMntPoint Pointer to the FsWrap_Mount_t structure
 *
 * @retval RET_STATE_SUCCESS on success;
 * @retval RET_STATE_ERR_PARAM if no system has been mounted at given mount point;
 * @retval RET_STATE_ERROR or an other errno code, depending on a file system back-end.
 */
RET_STATE_t FsWrap_Unmount(FsWrap_Mount_t* pMntPoint);

/**
 * @brief File or directory status
 *
 * Checks the status of a file or directory specified by the @p pPath.
 * @note The file on a storage device may not be updated until it is closed.
 *
 * @param pPath Path to the file or directory
 * @param pEntry Pointer to the zfs_dirent structure to fill if the file or
 * directory exists.
 *
 * @retval RET_STATE_SUCCESS on success;
 * @retval RET_STATE_ERR_PARAM when a bad directory or file name is given;
 * @retval RET_STATE_ERROR or an other errno code, depending on a file system back-end.
 */
RET_STATE_t FsWrap_Stat(const char* pPath, FsWrap_DirEnt_t* pEntry);

/**
 * @brief Retrieves statistics of the file system volume
 *
 * Returns the total and available space in the file system volume.
 *
 * @param pPath Path to the mounted directory
 * @param pStat Pointer to the zfs_statvfs structure to receive the fs
 * statistics.
 *
 * @retval RET_STATE_SUCCESS on success;
 * @retval RET_STATE_ERR_PARAM when a bad path to a directory, or a file, is given;
 * @retval RET_STATE_ERROR or an other errno code, depending on a file system back-end.
 */
RET_STATE_t FsWrap_StatVFS(const char* pPath, FsWrap_StatVfs_t* pStat);

/**
 * @brief Set label to disk
 *
 * @param pLabel Label string.
 *
 * @retval RET_STATE_SUCCESS on success;
 * @retval RET_STATE_ERROR or an other errno code.
 */
RET_STATE_t FsWrap_SetLabel(const char* pLabel);

/**
 * @brief Get label to disk
 *
 * @param pPath Logical drive number string.
 * @param pLabel Buffer to store the volume label.
 * @param pVsn Variable to store the volume serial number.
 *
 * @retval RET_STATE_SUCCESS on success;
 * @retval RET_STATE_ERROR or an other errno code
 */
RET_STATE_t FsWrap_GetLabel(const char* pPath, char* pLabel, u32* pVsn);

/**
 * @brief Create fresh file system
 *
 * @param fsType Type of file system to create.
 * @param devId Id of storage device.
 * @param pCfg Backend dependent init object. If NULL then default configuration is used.
 * @param flags Additional flags for file system implementation.
 *
 * @retval RET_STATE_SUCCESS on success;
 * @retval RET_STATE_ERROR or an other errno code, depending on a file system back-end.
 */
RET_STATE_t FsWrap_Mkfs(FS_TYPE_t fsType, char* devId, void* pCfg, u32 flags);

/**
 * @brief Register a file system
 *
 * Register file system with virtual file system.
 * Number of allowed file system types to be registered is controlled with the
 * FS_FILE_SYSTEM_MAX_TYPES option.
 *
 * @param type Type of file system (ex: @c FS_TYPE_FATFS)
 * @param pFs Pointer to File system
 *
 * @retval RET_STATE_SUCCESS on success;
 * @retval RET_STATE_ERROR when a file system of a given type has already been registered, or
 * when there is no space left, in file system registry, to add this file system type.
 */

RET_STATE_t FsWrap_Register(u32 type, const FsWrap_FileSystem_t* pFs);

/**
 * @brief Unregister a file system
 *
 * Unregister file system from virtual file system.
 *
 * @param type Type of file system (ex: @c FS_TYPE_FATFS)
 * @param pFs Pointer to File system
 *
 * @retval RET_STATE_SUCCESS on success;
 * @retval RET_STATE_ERROR when file system of a given type has not been registered.
 */
RET_STATE_t FsWrap_Unregister(u32 type, const FsWrap_FileSystem_t* pFs);

/**
 * @}
 */

#endif /* __FS_WRAPPER_H */
