#include "stringlib.h"

/* ------------------------------
 * JSON keys/format strings
 * ------------------------------ */
const char* JSON_KEY_TYPE	= "type";
const char* JSON_KEY_TSTAMP = "ts";
const char* JSON_VAL_TRUE	= "true";
const char* JSON_VAL_FALSE	= "false";

const char* JSON_FIELD_FIRST		= "{\"%s\":\"%s\",";
const char* JSON_FIELD_STR_STR		= "\"%s\":\"%s\",";
const char* JSON_FIELD_STR_INT		= "\"%s\":%d,";
const char* JSON_FIELD_STR_UINT		= "\"%s\":%u,";
const char* JSON_FIELD_STR_ULONG	= "\"%s\":%lu,";
const char* JSON_FIELD_STR_FLT		= "\"%s\":%f,";
const char* JSON_FIELD_STR_FLT2		= "\"%s\":%.2f,";
const char* JSON_FIELD_STR_BOOL		= "\"%s\":%s,";
const char* JSON_FIELD_STR_HEX_L	= "\"%s\":\"%x\",";
const char* JSON_FIELD_STR_HEX_U	= "\"%s\":\"%X\",";
const char* JSON_FIELD_STR_ARRAYSTR = "\"%s\":[%s],";
const char* JSON_FIELD_LAST			= "\"%s\":%lu}";

/* ------------------------------
 * Hex digits tables
 * ------------------------------ */
static const char HEX_DIGITS_LOWER[] = "0123456789abcdef";
static const char HEX_DIGITS_UPPER[] = "0123456789ABCDEF";

/* ------------------------------
 * Bytes <-> Hex
 * ------------------------------ */
void StringLib_BytesToHex(const u8* pBytes, u32 len, char* pOutHexStr, bool isUpper) {
	if (!pBytes || !pOutHexStr)
		return;

	const char* pTable = isUpper ? HEX_DIGITS_UPPER : HEX_DIGITS_LOWER;

	for (u32 i = 0; i < len; i++) {
		pOutHexStr[2 * i]	  = pTable[(pBytes[i] >> 4) & 0xF];
		pOutHexStr[2 * i + 1] = pTable[pBytes[i] & 0xF];
	}

	pOutHexStr[2 * len] = '\0';
}

s32 StringLib_HexToBytes(const char* pcHexStr, u32 hexLen, u8* pOutBytes, u32 outMaxLen) {
	if (!pcHexStr || !pOutBytes)
		return -4;

	if (hexLen % 2 != 0)
		return -1; /* invalid length */

	u32 outLen = hexLen / 2;
	if (outLen > outMaxLen)
		return -2; /* buffer too small */

	for (u32 i = 0; i < outLen; i++) {
		char hi = pcHexStr[2 * i];
		char lo = pcHexStr[2 * i + 1];

		int hiVal = (hi >= '0' && hi <= '9')   ? hi - '0'
					: (hi >= 'a' && hi <= 'f') ? hi - 'a' + 10
					: (hi >= 'A' && hi <= 'F') ? hi - 'A' + 10
											   : -1;

		int loVal = (lo >= '0' && lo <= '9')   ? lo - '0'
					: (lo >= 'a' && lo <= 'f') ? lo - 'a' + 10
					: (lo >= 'A' && lo <= 'F') ? lo - 'A' + 10
											   : -1;

		if (hiVal < 0 || loVal < 0)
			return -3; /* invalid char */

		pOutBytes[i] = (u8)((hiVal << 4) | loVal);
	}

	return (s32)outLen;
}

/* ------------------------------
 * U64 -> Hex
 * ------------------------------ */
void StringLib_U64ToHex(u64 val, char* pOutHexStr, u32 buffLen, bool isUpper) {
	if (!pOutHexStr)
		return;

	const char* pTable = isUpper ? HEX_DIGITS_UPPER : HEX_DIGITS_LOWER;

	if (buffLen < 2) {
		if (buffLen > 0)
			pOutHexStr[0] = '\0';

		return;
	}

	pOutHexStr[buffLen - 1] = '\0';
	char* pOut				= &pOutHexStr[buffLen - 2];

	do {
		*pOut-- = pTable[val % 16];
		val /= 16;
	} while (val && pOut >= pOutHexStr);

	u32 written = (u32)(&pOutHexStr[buffLen - 1] - (pOut + 1));
	memmove(pOutHexStr, pOut + 1, written);
	pOutHexStr[written] = '\0';
}

/* ------------------------------
 * Byte -> Binary text
 * ------------------------------ */
void StringLib_ByteToBin(u8 byte, char* pOutBinStr) {
	if (!pOutBinStr)
		return;

	for (int bit = 7; bit >= 0; --bit)
		*pOutBinStr++ = (byte & (1u << bit)) ? '1' : '0';

	*pOutBinStr = '\0';
}

/* ------------------------------
 * Path helpers
 * ------------------------------ */
char* StringLib_CutFilePath(char* pPath) {
	if (!pPath)
		return pPath;

	char* pPos = strrchr(pPath, '/');
	if (!pPos)
		pPos = strrchr(pPath, '\\');
	if (pPos)
		return pPos + 1;

	return pPath;
}

/* ------------------------------
 * Replace non printable
 * ------------------------------ */
void StringLib_ReplaceNonPrintable(char* pStr, u32 cnt) {
	if (!pStr)
		return;

	for (u32 i = 0; i < cnt; i++) {
		char c = pStr[i];
		if (c < 0x20 || c > 0x7E || c == '"' || c == '\'')
			pStr[i] = '.';
	}
}

/* ------------------------------
 * Fill buff (safe)
 * ------------------------------ */
void StringLib_FillBuff(char* pBuff, u32 buffSize, s32* pFreeBytesCnt, bool zeroFreeBytesIfNotFit,
						const char* pcFormatString, ...) {
	if (!pBuff || !pFreeBytesCnt || buffSize == 0)
		return;

	s32 freeBytes = *pFreeBytesCnt;
	if (freeBytes <= 0)
		return;

	if ((u32)freeBytes > buffSize)
		freeBytes = (s32)buffSize;

	u32 offset = (u32)buffSize - (u32)freeBytes;

	va_list args;
	va_start(args, pcFormatString);
	s32 r = vsnprintf(pBuff + offset, (u32)freeBytes, pcFormatString, args);
	va_end(args);

	if (r < 0) {
		if (zeroFreeBytesIfNotFit)
			*pFreeBytesCnt = -1;
		/* else leave freeBytes as-is */
	} else if ((u32)r >= (u32)freeBytes) {
		/* truncated */
		if (zeroFreeBytesIfNotFit)
			*pFreeBytesCnt = -1;
		else
			*pFreeBytesCnt = 0;
	} else {
		*pFreeBytesCnt = freeBytes - (s32)r;
	}
}

/* ------------------------------
 * Json check and pretty print
 * ------------------------------ */

/**
 * @see header documentation
 */
bool StringLib_JsonCheck(char* pIn, u32 inSize, char** ppOut) {
	if (!pIn || inSize < 2 || !ppOut)
		return false;

	char* pStart = NULL;
	char* pEnd	 = NULL;

	for (u32 i = 0; i < inSize; i++) {
		if (pIn[i] == '{' && pStart == NULL)
			pStart = &pIn[i];
		if (pIn[i] == '}')
			pEnd = &pIn[i];
	}

	if (!pStart || !pEnd || pStart > pEnd)
		return false;

	u32 endIdx = (u32)(pEnd - pIn);
	if (endIdx + 1 >= (u32)inSize) {
		/* no room to write terminating null */
		return false;
	}

	*ppOut		= pStart;
	*(pEnd + 1) = '\0';
	return true;
}

u32 StringLib_JsonPrettyPrint(const char* pсAnyJson, char* pFmtJson, u32 fmtJsonMaxLen, char quote,
							  u32 identLen, const char* pсNewLine, u32 startIdent) {
	u32 identLvl  = startIdent;
	bool inString = false;
	u32 n		  = 0;

	for (u32 i = 0; pсAnyJson[i] != '\0'; i++) {
		if (pсAnyJson[i] == quote) {
			inString = !inString;
		}

		if (!inString && pсAnyJson[i] != ' ') {
			if (pсAnyJson[i] == '{' || pсAnyJson[i] == '[') {
				identLvl++;
				n += snprintf(pFmtJson + n, fmtJsonMaxLen - n, "%c%s%*s", pсAnyJson[i], pсNewLine,
							  identLvl * identLen, "");

			} else if (pсAnyJson[i] == '}' || pсAnyJson[i] == ']') {
				identLvl--;
				n += snprintf(pFmtJson + n, fmtJsonMaxLen - n, "%s%*s%c", pсNewLine,
							  identLvl * identLen, "", pсAnyJson[i]);

			} else if (pсAnyJson[i] == ',') {
				n += snprintf(pFmtJson + n, fmtJsonMaxLen - n, ",%s%*s", pсNewLine,
							  identLvl * identLen, "");

			} else if (pсAnyJson[i] == ':') {
				n += snprintf(pFmtJson + n, fmtJsonMaxLen - n, ": ");

			} else {
				n += snprintf(pFmtJson + n, fmtJsonMaxLen - n, "%c", pсAnyJson[i]);
			}

		} else if (inString) {
			n += snprintf(pFmtJson + n, fmtJsonMaxLen - n, "%c", pсAnyJson[i]);
		}
	}

	if (identLvl == 0) {
		n += snprintf(pFmtJson + n, fmtJsonMaxLen - n, "%s", pсNewLine);
	}

	return identLvl;
}

s32 StringLib_FindClosedBracketAndTerminate(char* pBuff, u32 buffLen) {
	if (!pBuff || buffLen == 0)
		return -1;

	for (u32 i = 0; i < buffLen; i++) {
		if (pBuff[i] == '}') {
			if (i + 1 < buffLen) {
				pBuff[i + 1] = '\0';
				return (s32)i;
			} else {
				return -1;
			}
		}
	}

	return -1;
}
