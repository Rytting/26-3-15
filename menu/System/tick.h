#ifndef __TICK_H
#define __TICK_H

#include "stm32f10x.h"

/* TIM2-based millisecond free-running counter
   Call Tick_Init() once (before Question2_Init).
   TIM2_IRQHandler is defined in Tick.c ¡ª no need to add it elsewhere. */

void     Tick_Init(void);
uint32_t Tick_GetMs(void);

#endif
