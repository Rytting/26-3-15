#include "Laser.h"

#define RED_LASER_PIN    GPIO_Pin_13
#define GREEN_LASER_PIN  GPIO_Pin_14
#define LASER_PORT       GPIOB

void Laser_Init(void)
{
    GPIO_InitTypeDef GPIO_InitStructure;

    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOB, ENABLE);

    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;
    GPIO_InitStructure.GPIO_Pin = RED_LASER_PIN | GREEN_LASER_PIN;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_Init(LASER_PORT, &GPIO_InitStructure);

    GPIO_ResetBits(LASER_PORT, RED_LASER_PIN | GREEN_LASER_PIN);   // Ä¬ÈÏÈ«Ãð
}

void RedLaser_On(void)
{
    GPIO_SetBits(LASER_PORT, RED_LASER_PIN);
}

void RedLaser_Off(void)
{
    GPIO_ResetBits(LASER_PORT, RED_LASER_PIN);
}

void GreenLaser_On(void)
{
    GPIO_SetBits(LASER_PORT, GREEN_LASER_PIN);
}

void GreenLaser_Off(void)
{
    GPIO_ResetBits(LASER_PORT, GREEN_LASER_PIN);
}

void Laser_AllOff(void)
{
    GPIO_ResetBits(LASER_PORT, RED_LASER_PIN | GREEN_LASER_PIN);
}