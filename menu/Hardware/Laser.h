#ifndef __LASER_H
#define __LASER_H

#include "stm32f10x.h"

void Laser_Init(void);

void RedLaser_On(void);
void RedLaser_Off(void);

void GreenLaser_On(void);
void GreenLaser_Off(void);

void Laser_AllOff(void);

#endif