#ifndef __CHINESE_FONT_H
#define __CHINESE_FONT_H

#include "stm32f10x.h"

typedef struct
{
    char index[4];        // ºº×Ö
    uint8_t code[32];     // 16x16µãÕó
}ChineseFont16;

extern const ChineseFont16 ChineseFont[];

void TFT_ShowChinese(uint16_t x,uint16_t y,char *s,uint16_t color);

#endif