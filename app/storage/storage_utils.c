#include "storage_utils.h"
#include "ff.h"
#include "fs_wrapper.h"
#include "mem_wrapper.h"
#include "platform.h"
#include "storage.h"
#include "stringlib.h"

RET_STATE_t StorageUtils_FsSpeedTest(const char* pPath, float* pReadSpeed, float* pWriteSpeed) {
	ASSERT_CHECK(pReadSpeed != NULL);
	ASSERT_CHECK(pReadSpeed != NULL);

	if (!pReadSpeed || !pWriteSpeed)
		return RET_STATE_ERR_PARAM;

	FsWrap_File_t* pTestFile =
		MemWrap_Malloc(sizeof(FsWrap_File_t), __FILENAME__, __LINE__, MEM_ALLOC_UNLIM_TMO);
	if (!pTestFile)
		return RET_STATE_ERROR;
	memset(pTestFile, 0, sizeof(FsWrap_File_t));

	char testFilePath[FS_TEST_FILE_NAME_LEN];
	sprintf(testFilePath, "%s%s", pPath, "speed_test_file");

	u32 flags		= FS_MODE_CREATE_ALWAYS | FS_MODE_WRITE | FS_MODE_READ;
	RET_STATE_t res = FsWrap_Open(pTestFile, _TEXT(testFilePath), flags);
	if (res != RET_STATE_SUCCESS) {
		MemWrap_Free(pTestFile);
		return RET_STATE_ERROR;
	}

	u8* pBigBuff =
		(u8*)MemWrap_Malloc(FS_TEST_BUFF_SIZE, __FILENAME__, __LINE__, MEM_ALLOC_UNLIM_TMO);
	if (!pBigBuff) {
		MemWrap_Free(pTestFile);
		return RET_STATE_ERROR;
	}
	for (u32 i = 0; i < FS_TEST_BUFF_SIZE; i++) {
		pBigBuff[i] = i % 256;
	}

	u32 writeStartTs = PL_GET_MS_CNT();
	u32 wr			 = 0;
	for (u32 i = 0; i < FS_TEST_FILE_SIZE; i += FS_TEST_BUFF_SIZE) {
		FsWrap_Write(pTestFile, (void*)(pBigBuff), FS_TEST_BUFF_SIZE, &wr);
	}
	u32 writeStopTs = PL_GET_MS_CNT();

	res = FsWrap_Seek(pTestFile, FS_SEEK_START, 0);
	if (res != RET_STATE_SUCCESS) {
		MemWrap_Free(pTestFile);
		MemWrap_Free(pBigBuff);
		return RET_STATE_ERROR;
	}

	u32 readStartTs = PL_GET_MS_CNT();
	u32 rd			= 0;
	for (u32 i = 0; i < FS_TEST_FILE_SIZE; i += FS_TEST_BUFF_SIZE) {
		FsWrap_Read(pTestFile, (void*)pBigBuff, FS_TEST_BUFF_SIZE, &rd);
	}
	u32 readStopTs = PL_GET_MS_CNT();

	res = FsWrap_Close(pTestFile);
	if (res != RET_STATE_SUCCESS) {
		MemWrap_Free(pTestFile);
		MemWrap_Free(pBigBuff);
		return RET_STATE_ERROR;
	}

	res = FsWrap_Unlink(testFilePath);
	if (res != RET_STATE_SUCCESS) {
		MemWrap_Free(pBigBuff);
		MemWrap_Free(pTestFile);
		return RET_STATE_ERROR;
	}

	float mbInSec = (float)DELAY_1_SECOND * (float)(FS_TEST_FILE_SIZE) / (float)DATA_1_MBYTE;
	*pWriteSpeed  = mbInSec / (float)(writeStopTs - writeStartTs);
	*pReadSpeed	  = mbInSec / (float)(readStopTs - readStartTs);

	RET_STATE_t finalRetState = RET_STATE_SUCCESS;
	if (*pWriteSpeed < FS_TEST_SPEED_WRITE_MIN || *pReadSpeed < FS_TEST_SPEED_READ_MIN) {
		finalRetState = RET_STATE_WARNING;
	}

	MemWrap_Free(pBigBuff);
	MemWrap_Free(pTestFile);
	return finalRetState;
}
