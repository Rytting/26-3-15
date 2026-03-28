#ifndef __TFT_SPI_H
#define __TFT_SPI_H

#include "stm32f10x.h"

#define WHITE   0xFFFF
#define BLACK   0x0000
#define RED     0xF800
#define YELLOW  0xFFE0
#define BLUE    0x001F
#define CYAN    0x07FF
#define GREEN   0x07E0

#define TFT_WIDTH   320
#define TFT_HEIGHT  240

void TFT_Init(void);
void TFT_Clear(uint16_t color);
void TFT_ShowString(uint16_t x, uint16_t y, char *str, uint16_t color);
void TFT_ClearRect(uint16_t x, uint16_t y, uint16_t w, uint16_t h, uint16_t color);
void TFT_FillBlock(uint16_t x, uint16_t y, uint8_t size, uint16_t color);

//使用之前记得先写Delay_Init()！！

#endif
