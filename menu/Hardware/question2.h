#ifndef __QUESTION2_H
#define __QUESTION2_H

#include "stm32f10x.h"

typedef enum
{
    Q2_STOPPED = 0,
    Q2_PREPARE,     // 预备：锁框、去起点、可重锁
    Q2_READY,       // 已就绪，等确认启动
    Q2_RUNNING,     // 正式30秒
    Q2_PAUSED,
    Q2_DONE
} Q2_RunState;

typedef struct
{
    uint8_t rect_locked;
    uint8_t ready_to_start;
    uint8_t laser_found;

    int16_t x_left;
    int16_t x_right;
    int16_t y_top;
    int16_t y_bottom;

    int16_t tx;
    int16_t ty;

    int16_t bx;
    int16_t by;
} Q2_ViewData;

void Question2_Init(void);
void Question2_Reset(void);
void Question2_Task(void);
void Question2_ShowPage(void);

void Question2_EnterPrepare(void);
void Question2_StartFormal(void);
void Question2_Pause(void);
void Question2_Resume(void);
void Question2_Stop(void);
void Question2_Done(void);      /* 正式结束，冻结计时 */
void Question2_AutoDoneFromISR(void);  /* 仅供UART中断调用：置pending标志 */
void Question2_Relock(void);

Q2_RunState Question2_GetRunState(void);

void Question2_UpdateDiffData(int16_t dx, int16_t dy);
void Question2_UpdateViewData(int16_t xl, int16_t xr, int16_t yt, int16_t yb,
                              int16_t tx, int16_t ty,
                              int16_t bx, int16_t by,
                              uint8_t flags);

/* 新增：菜单高亮控制 */
void Question2_SetMenuSelect(uint8_t sel);
uint8_t Question2_GetMenuSelectMax(void);

#endif