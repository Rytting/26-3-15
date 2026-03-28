#include "stm32f10x.h"
#include "PWM.h"
#include "Servo.h"

static uint16_t Servo_AngleToCompare(float Angle)
{
    if(Angle < 0)   Angle = 0;
    if(Angle > 180) Angle = 180;

    return (uint16_t)(Angle / 180.0f * 2000.0f + 500.0f);
}

void Servo_Init(void)
{
    PWM_Init();

    /* 红云台回中 */
    Servo_SetAngle_X(90);
    Servo_SetAngle_Y(90);

    /* 绿云台回中 */
    Servo_SetAngle_GX(90);
    Servo_SetAngle_GY(90);
}

void Servo_SetAngle_X(float Angle)
{
    PWM_SetCompare1(Servo_AngleToCompare(Angle));   // PB6
}

void Servo_SetAngle_Y(float Angle)
{
    PWM_SetCompare2(Servo_AngleToCompare(Angle));   // PB7
}

void Servo_SetAngle_GX(float Angle)
{
    PWM_SetCompare3(Servo_AngleToCompare(Angle));   // PB8
}

void Servo_SetAngle_GY(float Angle)
{
    PWM_SetCompare4(Servo_AngleToCompare(Angle));   // PB9
}
