#ifndef __RAND_H
#define __RAND_H

#include "main.h"

void Rand_Init(void);
u32 Rand_GetNum(void);
void Rand_GetBuff(u32* pBuff, u32 maxNum);
s32 Rand_GetNumBetween(s32 min, s32 max);
bool Rand_GetBool(void);
void Rand_GetBuffBetween(u32* pBuff, u32 maxNum, s32 min, s32 max);
u32 Rand_GetStr(char* pBuff, u32 maxNum);

#endif /* __RAND_H */
