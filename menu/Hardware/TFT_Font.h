#ifndef __TFT_SPI_H
#define __TFT_SPI_H
#include "stm32f10x.h"

// 颜色
#define BLACK 0x0000
#define WHITE 0xFFFF

void TFT_Init(void);
void TFT_Clear(uint16_t color);
void TFT_DrawPoint(uint16_t x,uint16_t y,uint16_t color);
void TFT_ShowChar(uint16_t x,uint16_t y,uint8_t ch,uint16_t color);
void TFT_ShowString(uint16_t x,uint16_t y,char *str,uint16_t color);

// ? 新增（菜单必须）
void TFT_Fill(uint16_t x,uint16_t y,uint16_t w,uint16_t h,uint16_t color);

#endif