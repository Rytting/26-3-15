#ifndef __K230_UART_H
#define __K230_UART_H

#include "stm32f10x.h"

void K230_Uart_Init(void);

uint8_t K230_Uart_GetFrameFlag(void);
void K230_Uart_ClearFrameFlag(void);

int16_t K230_Uart_GetDx(void);
int16_t K230_Uart_GetDy(void);

void K230_Uart_SendString(char *str);
void K230_SetMode(uint8_t mode);
void K230_Run(uint8_t en);
void K230_Stop(void);
void K230_ResetVision(void);

#endif
