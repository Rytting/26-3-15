#ifndef __QUESTION2_H
#define __QUESTION2_H

#include "stm32f10x.h"

typedef enum
{
    Q2_STOPPED = 0,
    Q2_PREPARE,
    Q2_READY,
    Q2_RUNNING,
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

/* 生命周期 */
void Question2_Init(void);
void Question2_Reset(void);
void Question2_Task(void);
void Question2_ShowPage(void);

/* 状态控制 */
void Question2_EnterPrepare(void);   /* 内部会注册 UART 回调 */
void Question2_StartFormal(void);
void Question2_Pause(void);
void Question2_Resume(void);
void Question2_Stop(void);
void Question2_Done(void);           /* 正式结束，冻结计时 */
void Question2_Relock(void);

/* 查询 */
Q2_RunState Question2_GetRunState(void);

/* 菜单高亮 */
void    Question2_SetMenuSelect(uint8_t sel);
uint8_t Question2_GetMenuSelectMax(void);

/* 注意：UpdateDiffData / UpdateViewData / AutoDoneFromISR
   已改为内部 static 函数，外部不可调用，
   通过 K230_RegisterCallbacks 自动路由 */

#endif
