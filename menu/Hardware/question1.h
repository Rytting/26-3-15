#ifndef __QUESTION1_H
#define __QUESTION1_H

#include "stm32f10x.h"

typedef enum
{
    Q1_STOPPED = 0,
    Q1_RUNNING
} Q1_RunState;

void Question1_Init(void);
void Question1_Task(void);
void Question1_Reset(void);
void Question1_SetRunState(Q1_RunState state);
void Question1_ShowRunPage(void);
#endif