#ifndef __GIMBAL_PID_H
#define __GIMBAL_PID_H

#include "stm32f10x.h"

typedef struct
{
    float kp;
    float ki;
    float kd;

    float err;
    float err_last;
    float integral;

    float out;
    float out_limit;
    float integral_limit;
} PID_TypeDef;

void GimbalPID_Init(void);
void GimbalPID_Reset(void);
void GimbalPID_Update(int16_t dx, int16_t dy);

float GimbalPID_GetXAngle(void);
float GimbalPID_GetYAngle(void);

/* µ÷ÊÔ½Ó¿Ú */
float GimbalPID_GetXErr(void);
float GimbalPID_GetYErr(void);
float GimbalPID_GetXOut(void);
float GimbalPID_GetYOut(void);

void GimbalPID_SetParams(float kp_x, float ki_x, float kd_x,
                         float kp_y, float ki_y, float kd_y,
                         float i_lim_x, float i_lim_y);

#endif
