#ifndef __CRC_H
#define __CRC_H

#include "main.h"
#include "platform.h"
#include "platform_inc_m0.h"

bool CRC_Init(void);
void CRC_Reset(void);
u32 CRC_CheckBuff(const u8* pBuff, u32 buffSize);

#endif /* __CRC_H */
