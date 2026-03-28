#ifndef __PWM_H
#define __PWM_H

#include "stm32f10x.h"

void PWM_Init(void);

void PWM_SetCompare1(uint16_t Compare);   // PB6  TIM4_CH1  ºìX
void PWM_SetCompare2(uint16_t Compare);   // PB7  TIM4_CH2  ºìY
void PWM_SetCompare3(uint16_t Compare);   // PB8  TIM4_CH3  ÂÌX
void PWM_SetCompare4(uint16_t Compare);   // PB9  TIM4_CH4  ÂÌY

#endif