#include "Key.h"
#include "Delay.h"

#define KEY_UP_PIN    GPIO_Pin_10
#define KEY_DOWN_PIN  GPIO_Pin_11
#define KEY_OK_PIN    GPIO_Pin_12
#define KEY_PORT      GPIOB

void Key_Init(void)
{
    GPIO_InitTypeDef GPIO_InitStruct;
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOB, ENABLE);

    GPIO_InitStruct.GPIO_Mode = GPIO_Mode_IPU;
    GPIO_InitStruct.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_InitStruct.GPIO_Pin = KEY_UP_PIN | KEY_DOWN_PIN | KEY_OK_PIN;
    GPIO_Init(KEY_PORT, &GPIO_InitStruct);
}

Key_State Key_Scan(void)
{
    static uint8_t key_up_last = 1;
    static uint8_t key_down_last = 1;
    static uint8_t key_ok_last = 1;

    uint8_t key_up_now   = GPIO_ReadInputDataBit(KEY_PORT, KEY_UP_PIN);
    uint8_t key_down_now = GPIO_ReadInputDataBit(KEY_PORT, KEY_DOWN_PIN);
    uint8_t key_ok_now   = GPIO_ReadInputDataBit(KEY_PORT, KEY_OK_PIN);

    if(key_up_last == 1 && key_up_now == 0)
    {
        Delay_ms(20);
        if(GPIO_ReadInputDataBit(KEY_PORT, KEY_UP_PIN) == 0)
        {
            key_up_last = 0;
            return KEY_UP;
        }
    }
    else if(key_up_now == 1)
    {
        key_up_last = 1;
    }

    if(key_down_last == 1 && key_down_now == 0)
    {
        Delay_ms(20);
        if(GPIO_ReadInputDataBit(KEY_PORT, KEY_DOWN_PIN) == 0)
        {
            key_down_last = 0;
            return KEY_DOWN;
        }
    }
    else if(key_down_now == 1)
    {
        key_down_last = 1;
    }

    if(key_ok_last == 1 && key_ok_now == 0)
    {
        Delay_ms(20);
        if(GPIO_ReadInputDataBit(KEY_PORT, KEY_OK_PIN) == 0)
        {
            key_ok_last = 0;
            return KEY_OK;
        }
    }
    else if(key_ok_now == 1)
    {
        key_ok_last = 1;
    }

    return KEY_NONE;
}
