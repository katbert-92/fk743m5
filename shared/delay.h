#ifndef __DELAY_H
#define __DELAY_H

#include "main.h"

#define DELAY_MAX_TIME 0xFFFFFFFFU

void Delay_Init(void);
void Delay_WaitTime_MilliSec(u32 ms);
void Delay_WaitTime_MicroSec(u64 us);
u32 Delay_TimeMilliSec_Get(void);
u64 Delay_TimeMicroSec_Get(void);
double Delay_TimeAccurate_Get(void);
void Delay_SuspendTimer(void);
void Delay_ResumeTimer(void);

#endif /* __DELAY_H */
