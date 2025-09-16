#ifndef __STRING_LIB_H
#define __STRING_LIB_H

#include "main.h"

#ifdef __cplusplus
extern "C" {
#endif

/** --------------------------
 *  JSON common keys / formats
 *  -------------------------- */
extern const char* JSON_KEY_TYPE;
extern const char* JSON_KEY_TSTAMP;

extern const char* JSON_VAL_TRUE;
extern const char* JSON_VAL_FALSE;

extern const char* JSON_FIELD_FIRST;
extern const char* JSON_FIELD_STR_STR;
extern const char* JSON_FIELD_STR_INT;
extern const char* JSON_FIELD_STR_UINT;
extern const char* JSON_FIELD_STR_ULONG;
extern const char* JSON_FIELD_STR_FLT;
extern const char* JSON_FIELD_STR_FLT2;
extern const char* JSON_FIELD_STR_BOOL;
extern const char* JSON_FIELD_STR_HEX;
extern const char* JSON_FIELD_STR_ARRAYSTR;
extern const char* JSON_FIELD_LAST;

/* helper */
#define JSON_BOOL_VAL_GET(a) ((a) ? JSON_VAL_TRUE : JSON_VAL_FALSE)

/* Convenience: cut file path to filename */
#define __FILENAME__ StringLib_CutFilePath(__FILE__)

/* Pretty print helper macro (uses default quote '"' and ident=4, newline CRLF) */
#define STRING_LIB_JSON_PRETTY_PRINT_DEF(pAnyJson, pFmtJson, fmtJsonMaxLen) \
	StringLib_JsonPrettyPrint((pAnyJson), (pFmtJson), (fmtJsonMaxLen), '\"', 4, "\r\n", 0)

/** --------------------------
 *  Hex / Bytes conversions
 *  -------------------------- */

/**
 * @brief Convert bytes -> hex string
 * 
 * @param pBytes     input bytes buffer
 * @param len        number of bytes
 * @param pOutHexStr output buffer; must be at least 2*len + 1 bytes
 * @param isUpper    true => produce "ABCDEF", false => "abcdef"
 */
void StringLib_BytesToHex(const u8* pBytes, u32 len, char* pOutHexStr, bool isUpper);

/**
 * @brief Convert hex string -> bytes
 * 
 * @param pcHexStr    input hex buffer (no "0x", just hex characters)
 * @param hexLen     length of hex string (must be even)
 * @param pOutBytes  output buffer
 * @param outMaxLen  capacity of output buffer in bytes
 * @return s32       number of output bytes written, or negative error:
 *                   -1 invalid hex length (odd), -2 out buffer too small, -3 invalid char
 */
s32 StringLib_HexToBytes(const char* pcHexStr, u32 hexLen, u8* pOutBytes, u32 outMaxLen);

/**
 * @brief Convenience: treat ASCII string as bytes and convert to hex
 */
static inline void StringLib_StringToHex(const char* str, u32 strLen, char* pOutHexStr,
										 bool isUpper) {
	StringLib_BytesToHex((const u8*)str, strLen, pOutHexStr, isUpper);
}

/**
 * @brief Convenience: hex->string (writes raw bytes into outStr)
 * @return s32 as StringLib_HexToBytes
 */
static inline s32 StringLib_HexToString(const char* pcHexStr, u32 hexLen, char* pOutStr,
										u32 outMaxLen) {
	return StringLib_HexToBytes(pcHexStr, hexLen, (u8*)pOutStr, outMaxLen);
}

/** --------------------------
 *  u64 -> hex
 *  -------------------------- */

/**
 * @brief Convert unsigned 64-bit integer to hexadecimal string
 * 
 * @param val        value
 * @param pOutHexStr output buffer
 * @param buffLen    buffer length in bytes (including terminating '\0')
 * @param isUpper    uppercase hex if true
 * 
 * If buffLen is too small, function writes empty string in pOutHexStr (if buffLen>0).
 */
void StringLib_U64ToHex(u64 val, char* pOutHexStr, u32 buffLen, bool isUpper);

/** --------------------------
 *  Byte -> binary text
 *  -------------------------- */

/**
 * @brief Convert one byte to "01010101" string (8 chars + '\0')
 * 
 * @param byte       input byte
 * @param pOutBinStr output buffer, must be >= 9 bytes
 */
void StringLib_ByteToBin(u8 byte, char* pOutBinStr);

/** --------------------------
 *  Misc string helpers
 *  -------------------------- */

/**
 * @brief Return pointer to filename part of a path (after last '/' or '\\')
 * 
 * The function returns pointer into pPath (does not allocate).
 */
char* StringLib_CutFilePath(char* pPath);

/**
 * @brief Replace non-printable characters and quotes in buffer with '.'
 * 
 * @param pStr pointer to buffer
 * @param cnt  number of bytes to process
 */
void StringLib_ReplaceNonPrintable(char* pStr, u32 cnt);

/**
 * @brief Append formatted text into a buffer region with free-bytes accounting.
 * 
 * NOTE: this function updates *pFreeBytesCnt (signed) to reflect remaining free bytes.
 * 
 * @param pBuff Pointer to buffer (whole buffer).
 * @param buffSize Entire buffer size (bytes).
 * @param pFreeBytesCnt Pointer to remaining free bytes (signed). On entry it must be <= buffSize.
 *                       On return it will be updated:
 *                         - >=0 number of free bytes left
 *                         - -1 indicates overflow/truncation if zeroFreeBytesIfNotFit==true
 * @param zeroFreeBytesIfNotFit if true and output doesn't fit, *pFreeBytesCnt becomes -1
 * @param pcFormatString printf-like format
 */
void StringLib_FillBuff(char* pBuff, u32 buffSize, s32* pFreeBytesCnt, bool zeroFreeBytesIfNotFit,
						const char* pcFormatString, ...);

/** --------------------------
 *  JSON helpers
 *  -------------------------- */

/**
 * @brief Locate JSON object in buffer and null-terminate it right after '}'.
 * 
 * IMPORTANT: This function modifies the input buffer by writing '\0' after the closing brace.
 * 
 * @param pIn    input buffer (mutable). Contains arbitrary data, may include JSON somewhere inside.
 * @param inSize size of the buffer (bytes)
 * @param ppOut  out parameter; on success *ppOut points to the start of JSON ('{')
 * @return bool  true on success (and buffer is terminated at position after '}'), false otherwise
 */
bool StringLib_JsonCheck(char* pIn, u32 inSize, char** ppOut);

/**
 * @brief Pretty-print JSON into a formatted buffer.
 * 
 * The function is robust to escaped quotes (\"), will not overflow pFmtJson, and returns
 * number of characters written (excluding terminating NUL) or -1 on error.
 * 
 * @param pсAnyJson     input JSON string (null-terminated)
 * @param pFmtJson      output buffer
 * @param fmtJsonMaxLen size of output buffer in bytes
 * @param quote         quote char used in JSON ('"' or '\'')
 * @param identLen      number of spaces per indent level
 * @param pсNewLine     newline string (e.g. "\n" or "\r\n")
 * @param startIdent    starting indentation level (useful for chunked formatting)
 * @return u32          number of bytes written (excluding final '\0')
 */
u32 StringLib_JsonPrettyPrint(const char* pсAnyJson, char* pFmtJson, u32 fmtJsonMaxLen, char quote,
							  u32 identLen, const char* pсNewLine, u32 startIdent);

/**
 * @brief Find closing '}' and terminate the buffer after it.
 * 
 * @param pBuff   buffer to search (mutable)
 * @param buffLen buffer size
 * @return s32 index of '}' on success (>=0), -1 on failure (not found or no room to terminate)
 */
s32 StringLib_FindClosedBracketAndTerminate(char* pBuff, u32 buffLen);

#ifdef __cplusplus
}
#endif

#endif /* __STRING_LIB_H */
