#ifndef __K230_UART_H
#define __K230_UART_H

#include "stm32f10x.h"

/* =========================
   回调函数类型
   ========================= */

/* DIF 误差帧：每帧视觉误差 (dx, dy)，由当前活跃题目处理 */
typedef void (*K230_DiffCallback)(int16_t dx, int16_t dy);

/* Q2V 视图帧：Q2 专用画布数据 */
typedef void (*K230_Q2ViewCallback)(int16_t xl, int16_t xr,
                                    int16_t yt, int16_t yb,
                                    int16_t tx, int16_t ty,
                                    int16_t bx, int16_t by,
                                    uint8_t flags);

/* STATE,DONE 帧：K230 通知一圈跑完，由当前活跃题目处理 */
typedef void (*K230_AutoDoneCallback)(void);

/* =========================
   回调注册
   每道题在 EnterPrepare() 里调一次，
   切题时自动覆盖，不需要手动注销
   ========================= */
void K230_RegisterCallbacks(K230_DiffCallback     diff_cb,
                             K230_Q2ViewCallback   q2view_cb,
                             K230_AutoDoneCallback done_cb);

/* =========================
   基础接口（保持不变）
   ========================= */
void K230_Uart_Init(void);

void K230_Uart_SendString(char *str);
void K230_SetMode(uint8_t mode);
void K230_Run(uint8_t en);
void K230_Stop(void);
void K230_ResetVision(void);

#endif
