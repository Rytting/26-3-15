#include "Question1.h"
#include "K230_UART.h"
#include "Servo.h"
#include "GimbalPID.h"
#include "TFT_SPI.h"
#include "Delay.h"
#include "Laser.h"
#include <stdio.h>

static Q1_RunState g_q1_state = Q1_STOPPED;

/* 最近一次接收到的偏差 */
static int16_t g_last_dx = 0;
static int16_t g_last_dy = 0;
static uint8_t g_has_data = 0;

/* 只做一次底层初始化 */
static uint8_t g_q1_hw_inited = 0;

/* 内部显示函数 */
static void Q1_ShowBaseUI(void);
static void Q1_ShowState(Q1_RunState state);
static void Q1_ShowData(int16_t dx, int16_t dy);
static void Q1_ShowNoData(void);

void Question1_Init(void)
{
    /* 1. 只初始化一次底层硬件 */
    if(g_q1_hw_inited == 0)
    {
        K230_Uart_Init();
        Servo_Init();
        GimbalPID_Init();
		Laser_Init();
        g_q1_hw_inited = 1;
    }

    /* 2. 本题状态清零 */
    g_q1_state = Q1_STOPPED;
    g_last_dx = 0;
    g_last_dy = 0;
    g_has_data = 0;
    K230_Uart_ClearFrameFlag();

    /* 3. 让K230切到Q1模式，并先停止 */
    K230_SetMode(1);
    Delay_ms(50);
    K230_ResetVision();
    Delay_ms(50);
    K230_Stop();

    /* 4. 本地控制复位到中位 */
    GimbalPID_Reset();

    /* 5. 画界面 */
    TFT_Clear(WHITE);
    Q1_ShowBaseUI();
    Q1_ShowState(Q1_STOPPED);
    Q1_ShowNoData();
}

void Question1_SetRunState(Q1_RunState state)
{
    g_q1_state = state;

    if(state == Q1_RUNNING)
    {
        K230_Uart_ClearFrameFlag();

        K230_SetMode(1);
        Delay_ms(20);
        K230_Run(1);

        RedLaser_On();     

        Q1_ShowState(Q1_RUNNING);
    }
    else
    {
        K230_Stop();
        RedLaser_Off();  

        Q1_ShowState(Q1_STOPPED);
    }
}

void Question1_Reset(void)
{
    g_q1_state = Q1_STOPPED;

    /* 停止视觉 */
    K230_Stop();
    Delay_ms(20);
	
	/* 关激光 */
	RedLaser_Off();
	
    /* 本地回中 */
    GimbalPID_Reset();

    /* 视觉复位 */
    K230_SetMode(1);
    Delay_ms(20);
    K230_ResetVision();

    /* 清状态 */
    g_last_dx = 0;
    g_last_dy = 0;
    g_has_data = 0;
    K230_Uart_ClearFrameFlag();

    /* 5. 刷界面 */
    TFT_Clear(WHITE);
    Q1_ShowBaseUI();
    Q1_ShowState(Q1_STOPPED);
    Q1_ShowNoData();
}

void Question1_Task(void)
{
    char buf[40];
    float x_ang, y_ang;

    if(g_q1_state != Q1_RUNNING)
    {
        return;
    }

    if(K230_Uart_GetFrameFlag())
    {
        K230_Uart_ClearFrameFlag();

        g_last_dx = K230_Uart_GetDx();
        g_last_dy = K230_Uart_GetDy();
        g_has_data = 1;

        GimbalPID_Update(g_last_dx, g_last_dy);

        x_ang = GimbalPID_GetXAngle();
        y_ang = GimbalPID_GetYAngle();

        Q1_ShowData(g_last_dx, g_last_dy);

        TFT_ClearRect(10, 130, 220, 90, WHITE);

        sprintf(buf, "XANG=%.1f   ", x_ang);
        TFT_ShowString(10, 130, buf, BLACK);

        sprintf(buf, "YANG=%.1f   ", y_ang);
        TFT_ShowString(10, 160, buf, BLACK);

        sprintf(buf, "XOUT=%.2f   ", GimbalPID_GetXOut());
        TFT_ShowString(10, 190, buf, BLUE);

        sprintf(buf, "YOUT=%.2f   ", GimbalPID_GetYOut());
        TFT_ShowString(10, 220, buf, BLUE);
    }
}

/* =========================
   内部显示函数
   ========================= */

static void Q1_ShowBaseUI(void)
{
    TFT_ShowString(10, 10,  "Q1: RESET TO ORIGIN", RED);
    TFT_ShowString(10, 40,  "K230 MODE=1", BLACK);
    TFT_ShowString(10, 70,  "STATE:", BLACK);
    TFT_ShowString(10, 100, "DX=---", BLACK);
    TFT_ShowString(140,100, "DY=---", BLACK);
}

static void Q1_ShowState(Q1_RunState state)
{
    TFT_ClearRect(90, 70, 150, 25, WHITE);

    if(state == Q1_RUNNING)
    {
        TFT_ShowString(90, 70, "RUNNING", RED);
    }
    else
    {
        TFT_ShowString(90, 70, "STOPPED", BLUE);
    }
}

static void Q1_ShowData(int16_t dx, int16_t dy)
{
    char buf[24];

    TFT_ClearRect(10, 100, 280, 25, WHITE);

    sprintf(buf, "DX=%d   ", dx);
    TFT_ShowString(10, 100, buf, BLACK);

    sprintf(buf, "DY=%d   ", dy);
    TFT_ShowString(140, 100, buf, BLACK);
}

static void Q1_ShowNoData(void)
{
    TFT_ClearRect(10, 100, 280, 120, WHITE);
    TFT_ShowString(10, 100, "DX=---", BLACK);
    TFT_ShowString(140,100, "DY=---", BLACK);
    TFT_ShowString(10, 130, "NO DATA", BLUE);
}

void Question1_ShowRunPage(void)
{
    TFT_Clear(WHITE);
    Q1_ShowBaseUI();
    Q1_ShowState(g_q1_state);

    if(g_has_data)
    {
        char buf[40];

        Q1_ShowData(g_last_dx, g_last_dy);

        sprintf(buf, "XANG=%.1f   ", GimbalPID_GetXAngle());
        TFT_ShowString(10, 130, buf, BLACK);

        sprintf(buf, "YANG=%.1f   ", GimbalPID_GetYAngle());
        TFT_ShowString(10, 160, buf, BLACK);

        sprintf(buf, "XOUT=%.2f   ", GimbalPID_GetXOut());
        TFT_ShowString(10, 190, buf, BLUE);

        sprintf(buf, "YOUT=%.2f   ", GimbalPID_GetYOut());
        TFT_ShowString(10, 220, buf, BLUE);
    }
    else
    {
        Q1_ShowNoData();
    }
}
