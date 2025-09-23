#ifndef __RNG_H
#define __RNG_H

#include "main.h"
#include "platform.h"
#include "platform_inc_m0.h"

bool RNG_Init(void);
bool RNG_GenerateBuff(u32* pBuff, u32 maxNum);

#endif /* __RNG_H */
