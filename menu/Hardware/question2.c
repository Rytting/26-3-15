#include "Question2.h"
#include "K230_UART.h"
#include "Servo.h"
#include "GimbalPID.h"
#include "TFT_SPI.h"
#include "Delay.h"
#include "Tick.h"
#include <stdio.h>
#include <string.h>
#include "Laser.h"

/* =========================
   1) K230 ROI / 安全边框
   当前按 q2.py 参数写死
   ROI: 240 x 300
   MARGIN = 13
   ========================= */
#define Q2_ROI_W       240
#define Q2_ROI_H       300
#define Q2_SAFE_L      13
#define Q2_SAFE_T      13
#define Q2_SAFE_R      (Q2_ROI_W - 13)
#define Q2_SAFE_B      (Q2_ROI_H - 13)

/* =========================
   2) TFT 画布区域
   ========================= */
#define CANVAS_X       18
#define CANVAS_Y       18
#define CANVAS_W       150
#define CANVAS_H       190

/* =========================
   3) 右侧文字区域
   ========================= */
#define TEXT_X         185
#define TEXT_Y         18

/* =========================
   4) 点大小
   ========================= */
#define DOT_SIZE       4

/* =========================
   5) Q2V flags
   bit0 = rect_locked
   bit1 = ready_to_start
   bit2 = laser_found
   ========================= */
#define Q2_FLAG_RECT_LOCKED   0x01
#define Q2_FLAG_READY         0x02
#define Q2_FLAG_LASER_FOUND   0x04

static Q2_RunState g_q2_state = Q2_STOPPED;
static uint8_t g_q2_hw_inited = 0;

static Q2_ViewData g_view;
static volatile  uint8_t g_has_view = 0;

static volatile  int16_t g_last_dx = 0;
static volatile int16_t g_last_dy = 0;
static volatile  uint8_t g_has_diff = 0;

static uint8_t  g_q2_menu_sel = 0;   /* 0/1/2 */

/* =========================
   计时相关
   ========================= */
static uint32_t g_run_start_ms  = 0;   /* StartFormal 时记录的 Tick 值 */
static uint32_t g_run_final_ms  = 0;   /* Done/Stop 时冻结的用时 (ms)  */
/* 上次刷到屏幕的 0.1s 单位；0xFFFF 表示需要强制刷新 */
static uint16_t g_disp_sec10    = 0xFFFF;

/* K230 自动发 DONE 时，中断里只置这个标志
   实际的 Done 动作在 Task() 里执行，避免在中断里做阻塞串口发送 */
static volatile uint8_t g_auto_done_pending = 0;
/* =========================
   6) 显示状态管理
   ========================= */
/* 置1表示需要整屏重绘（状态切换、relock等） */
static uint8_t  g_ui_dirty   = 1;
/* 置1表示静态层已含已锁画布，可以做局部刷新 */
static uint8_t  g_rect_drawn = 0;
/* 上次刷到屏幕的DX/DY，用于判断是否需要更新文字 */
static int16_t  g_disp_dx    = 0x7FFF;
static int16_t  g_disp_dy    = 0x7FFF;

/* 方便统一打dirty标记 */
#define Q2_MARK_DIRTY()  do { g_ui_dirty = 1; g_rect_drawn = 0; } while(0)

/* =========================
   7) 内部函数声明
   ========================= */
static void Q2_ClearViewData(void);

static void Q2_SendPrepareCmd(void);
static void Q2_SendStartCmd(void);
static void Q2_SendRelockCmd(void);

static void Q2_DrawHLine(uint16_t x, uint16_t y, uint16_t w, uint16_t color);
static void Q2_DrawVLine(uint16_t x, uint16_t y, uint16_t h, uint16_t color);
static void Q2_DrawRect(uint16_t x, uint16_t y, uint16_t w, uint16_t h, uint16_t color);
static void Q2_DrawPoint(uint16_t x, uint16_t y, uint8_t size, uint16_t color);

static uint16_t Q2_MapX(int16_t x);
static uint16_t Q2_MapY(int16_t y);

static void Q2_ShowWords(uint16_t x, uint16_t y, const char *text, uint16_t color);
static void Q2_ShowStateText(void);
static void Q2_ShowHintText(void);
static void Q2_ShowOneOption(uint16_t x, uint16_t y, const char *text, uint8_t selected, uint16_t color);

static void Q2_DrawStaticUI(void);   /* 全屏重绘，只在 g_ui_dirty 时调用 */
static void Q2_UpdateCanvas(void);   /* 仅刷画布内部（点+框），每帧调用  */
static void Q2_UpdateDXDY(void);     /* 仅更新右侧DX/DY数字，值变时调用  */
static void Q2_UpdateTimer(void);    /* 仅更新计时，每 0.1s 刷一次       */
static uint32_t Q2_GetElapsedMs(void); /* 取当前或冻结用时               */

/* UART 回调前向声明，供 EnterPrepare 注册使用 */
static void Q2_OnDiff(int16_t dx, int16_t dy);
static void Q2_OnViewData(int16_t xl, int16_t xr, int16_t yt, int16_t yb,
                           int16_t tx, int16_t ty,
                           int16_t bx, int16_t by, uint8_t flags);
static void Q2_AutoDoneFromISR(void);

/* =========================
   8) 对外接口
   ========================= */
void Question2_Init(void)
{
    if(g_q2_hw_inited == 0)
    {
        K230_Uart_Init();
        Servo_Init();
        GimbalPID_Init();
        Laser_Init();
        g_q2_hw_inited = 1;
    }

    Question2_Reset();
}

void Question2_Reset(void)
{
    g_q2_state = Q2_STOPPED;

    g_last_dx = 0;
    g_last_dy = 0;
    g_has_diff = 0;

    Q2_ClearViewData();

    K230_Stop();
    Delay_ms(20);
	
	Laser_AllOff();
	
    GimbalPID_Reset();

    K230_SetMode(2);
    Delay_ms(30);
    K230_ResetVision();
    Delay_ms(30);
    K230_Stop();

    Q2_MARK_DIRTY();
}

void Question2_EnterPrepare(void)
{
    g_q2_state = Q2_PREPARE;
    g_has_diff = 0;

    /* 注册本题的 UART 回调，覆盖其他题目之前注册的 */
    K230_RegisterCallbacks(Q2_OnDiff, Q2_OnViewData, Q2_AutoDoneFromISR);

    GimbalPID_Reset();
    GimbalPID_SetParams(
        0.05f,  0.001f, 0.08f,
        0.03f,  0.001f, 0.10f,
        30.0f, 80.0f
    );
    Q2_ClearViewData();

    K230_SetMode(2);
    Delay_ms(20);

    Q2_SendPrepareCmd();
    Delay_ms(20);
	
	RedLaser_On();     
    GreenLaser_Off();  
	
    K230_Run(1);

    Q2_MARK_DIRTY();
}

void Question2_StartFormal(void)
{
    if(g_q2_state != Q2_READY && g_q2_state != Q2_PREPARE)
        return;

    g_q2_state = Q2_RUNNING;
	
	RedLaser_On();     
    GreenLaser_Off(); 

    /* 记录起始时间，强制刷新计时显示 */
    g_run_start_ms = Tick_GetMs();
    g_run_final_ms = 0;
    g_disp_sec10   = 0xFFFF;
	
    Q2_SendStartCmd();

    Q2_MARK_DIRTY();
}

void Question2_Pause(void)
{
    if(g_q2_state == Q2_RUNNING)
    {
        g_q2_state = Q2_PAUSED;
        K230_Stop();

        Q2_MARK_DIRTY();
    }
}

void Question2_Resume(void)
{
    if(g_q2_state == Q2_PAUSED)
    {
        g_q2_state = Q2_RUNNING;
        K230_Run(1);

        Q2_MARK_DIRTY();
    }
}

void Question2_Stop(void)
{
    /* 如果正在计时，先冻结用时 */
    if(g_q2_state == Q2_RUNNING || g_q2_state == Q2_PAUSED)
        g_run_final_ms = Tick_GetMs() - g_run_start_ms;

    g_q2_state = Q2_STOPPED;
    K230_Stop();

    Q2_MARK_DIRTY();
}

void Question2_Done(void)
{
    if(g_q2_state != Q2_RUNNING && g_q2_state != Q2_PAUSED)
        return;

    /* 冻结用时 */
    g_run_final_ms = Tick_GetMs() - g_run_start_ms;
    g_disp_sec10   = 0xFFFF;

    g_q2_state = Q2_DONE;
    K230_Stop();

    Q2_MARK_DIRTY();
}

/* 仅供回调：只置 pending 标志，
   不在中断里做任何 TFT / 串口阻塞操作 */
static void Q2_AutoDoneFromISR(void)
{
    if(g_q2_state == Q2_RUNNING)
        g_auto_done_pending = 1;
}

void Question2_Relock(void)
{
    if(g_q2_state == Q2_PREPARE || g_q2_state == Q2_READY)
    {
        Q2_ClearViewData();   /* 内部已含 MARK_DIRTY */
        g_has_diff = 0;
        GimbalPID_Reset();

        Q2_SendRelockCmd();

        Q2_MARK_DIRTY();
    }
}

Q2_RunState Question2_GetRunState(void)
{
    return g_q2_state;
}

void Question2_SetMenuSelect(uint8_t sel)
{
    if(sel > 2) sel = 2;
    g_q2_menu_sel = sel;
    Q2_MARK_DIRTY();
}

uint8_t Question2_GetMenuSelectMax(void)
{
    return 2;
}
/* 三个函数只由 UART 回调调用，不对外暴露 */
static void Q2_OnDiff(int16_t dx, int16_t dy)
{
    g_last_dx = dx;
    g_last_dy = dy;
    g_has_diff = 1;
}

static void Q2_OnViewData(int16_t xl, int16_t xr, int16_t yt, int16_t yb,
                           int16_t tx, int16_t ty,
                           int16_t bx, int16_t by,
                           uint8_t flags)
{
    uint8_t was_locked = g_view.rect_locked;

    g_view.x_left   = xl;
    g_view.x_right  = xr;
    g_view.y_top    = yt;
    g_view.y_bottom = yb;

    g_view.tx = tx;
    g_view.ty = ty;

    g_view.bx = bx;
    g_view.by = by;

    g_view.rect_locked    = (flags & Q2_FLAG_RECT_LOCKED) ? 1 : 0;
    g_view.ready_to_start = (flags & Q2_FLAG_READY) ? 1 : 0;
    g_view.laser_found    = (flags & Q2_FLAG_LASER_FOUND) ? 1 : 0;

    g_has_view = 1;

    /* 刚完成锁框：触发一次全屏重绘，把静态绿框画上去 */
    if(!was_locked && g_view.rect_locked)
    {
        Q2_MARK_DIRTY();
    }

    if(g_q2_state == Q2_PREPARE && g_view.ready_to_start)
    {
        g_q2_state = Q2_READY;
        Q2_MARK_DIRTY();   /* 状态字变了，右侧文字要更新 */
    }
}

/* =========================
   9) Task：核心调度
   ========================= */
void Question2_Task(void)
{
    /* 中断里置的自动完成标志，在主循环里安全处理 */
    if(g_auto_done_pending)
    {
        g_auto_done_pending = 0;
        Question2_Done();   /* 现在在主循环里调，可以安全做串口发送和TFT */
    }

    if((g_q2_state == Q2_PREPARE ||
        g_q2_state == Q2_READY   ||
        g_q2_state == Q2_RUNNING)
       && g_has_diff)
    {
        GimbalPID_Update(g_last_dx, g_last_dy);
        g_has_diff = 0;
    }

    if(g_ui_dirty)
    {
        Q2_DrawStaticUI();
        g_ui_dirty = 0;
        g_disp_dx = 0x7FFF;
        g_disp_dy = 0x7FFF;
    }
    else if(g_rect_drawn && (g_q2_state == Q2_PAUSED ||
                              g_q2_state == Q2_PREPARE ||
                              g_q2_state == Q2_READY))
    {
        Q2_UpdateCanvas();   /* 只在有画布的状态下刷点 */
    }

    /* DX/DY 只在暂停/预备时有意义（运行时画布不显示） */
    if(g_q2_state == Q2_PAUSED  ||
       g_q2_state == Q2_PREPARE ||
       g_q2_state == Q2_READY)
    {
        Q2_UpdateDXDY();
    }

    /* 计时刷新：每 0.1s 更新一次，不占 PID 带宽 */
    if(g_q2_state == Q2_RUNNING || g_q2_state == Q2_DONE)
    {
        Q2_UpdateTimer();
    }
}

/* =========================
   10) ShowPage：公共接口保留
   外部调用只触发一次全屏重绘
   ========================= */
void Question2_ShowPage(void)
{
    Q2_MARK_DIRTY();
}

/* =========================
   11) 串口控制命令
   ========================= */
static void Q2_SendPrepareCmd(void)
{
    K230_Uart_SendString("$Q2PREP,1*\r\n");
}

static void Q2_SendStartCmd(void)
{
    K230_Uart_SendString("$Q2GO,1*\r\n");
}

static void Q2_SendRelockCmd(void)
{
    K230_Uart_SendString("$Q2RELOCK,1*\r\n");
}

/* =========================
   12) 全屏静态层绘制
   只在g_ui_dirty时调用一次
   ========================= */
static void Q2_DrawStaticUI(void)
{
    TFT_Clear(BLACK);

    /* 右侧：状态文字 + 操作提示 */
    Q2_ShowStateText();
    Q2_ShowHintText();

    if(g_q2_state == Q2_RUNNING || g_q2_state == Q2_DONE)
    {
        /* ---- 运行/完成：左侧画布区改为计时器区域 ---- */
        TFT_ClearRect(CANVAS_X, CANVAS_Y, CANVAS_W, CANVAS_H, BLACK);

        if(g_q2_state == Q2_RUNNING)
        {
            TFT_ShowString(CANVAS_X + 28, CANVAS_Y + 16, "RUNNING", RED);
        }
        else
        {
            TFT_ShowString(CANVAS_X + 38, CANVAS_Y + 16, "DONE!", GREEN);
        }

        /* 固定文字：TIME 标签 + 限时提示 */
        TFT_ShowString(CANVAS_X + 44, CANVAS_Y + 72, "TIME:", WHITE);
        /* 数值由 Q2_UpdateTimer() 填入 CANVAS_Y+100 行 */
        TFT_ShowString(CANVAS_X + 14, CANVAS_Y + 148,
                       "LIMIT: 30 s",
                       g_q2_state == Q2_RUNNING ? YELLOW : WHITE);

        /* 强制 UpdateTimer 在本帧后立刻刷一次数值 */
        g_disp_sec10 = 0xFFFF;
        g_rect_drawn = 0;  /* 无画布，不触发 UpdateCanvas */
    }
    else if(g_view.rect_locked)
    {
        /* ---- 预备/就绪/暂停 + 已锁框：画画布 ---- */
        TFT_ClearRect(CANVAS_X, CANVAS_Y, CANVAS_W, CANVAS_H, WHITE);
        Q2_DrawRect(CANVAS_X, CANVAS_Y, CANVAS_W, CANVAS_H, BLACK);

        {
            uint16_t xl = Q2_MapX(g_view.x_left);
            uint16_t xr = Q2_MapX(g_view.x_right);
            uint16_t yt = Q2_MapY(g_view.y_top);
            uint16_t yb = Q2_MapY(g_view.y_bottom);
            if(xr > xl && yb > yt)
                Q2_DrawRect(xl, yt, xr - xl, yb - yt, GREEN);
        }

        g_rect_drawn = 1;
    }
    else
    {
        /* ---- 未锁框 ---- */
        TFT_ClearRect(CANVAS_X, CANVAS_Y, CANVAS_W, CANVAS_H, BLACK);
        TFT_ShowString(CANVAS_X + 4, CANVAS_Y + 60,  "WAITING FOR", YELLOW);
        TFT_ShowString(CANVAS_X + 4, CANVAS_Y + 82,  "RECT LOCK",   YELLOW);
        TFT_ShowString(CANVAS_X + 4, CANVAS_Y + 104, "...",         YELLOW);
        g_rect_drawn = 0;
    }
}

/* =========================
   13) 局部刷新：只更新画布内部
   每帧调用，刷点的位置
   ========================= */
static void Q2_UpdateCanvas(void)
{
    /* 清画布内部（保留1px边框） */
    TFT_ClearRect(CANVAS_X + 1,
                  CANVAS_Y + 1,
                  CANVAS_W - 2,
                  CANVAS_H - 2,
                  WHITE);

    /* 重绘绿色锁框（它在内部，会被上面的clear抹掉） */
    if(g_has_view && g_view.rect_locked)
    {
        uint16_t xl = Q2_MapX(g_view.x_left);
        uint16_t xr = Q2_MapX(g_view.x_right);
        uint16_t yt = Q2_MapY(g_view.y_top);
        uint16_t yb = Q2_MapY(g_view.y_bottom);
        if(xr > xl && yb > yt)
            Q2_DrawRect(xl, yt, xr - xl, yb - yt, GREEN);
    }

    /* 蓝点：目标点 */
    if(g_has_view)
    {
        uint16_t tx = Q2_MapX(g_view.tx);
        uint16_t ty = Q2_MapY(g_view.ty);
        Q2_DrawPoint(tx, ty, DOT_SIZE, BLUE);
    }

    /* 红点：激光点 */
    if(g_has_view && g_view.laser_found)
    {
        uint16_t bx = Q2_MapX(g_view.bx);
        uint16_t by = Q2_MapY(g_view.by);
        Q2_DrawPoint(bx, by, DOT_SIZE, RED);
    }
}

/* =========================
   14) 局部刷新：只更新DX/DY数字
   值没变时跳过，避免闪烁
   ========================= */
static void Q2_UpdateDXDY(void)
{
    char buf[24];

    if(g_last_dx == g_disp_dx && g_last_dy == g_disp_dy)
        return;

    TFT_ClearRect(185, 170, 130, 44, BLACK);

    sprintf(buf, "DX=%d   ", g_last_dx);
    TFT_ShowString(185, 170, buf, WHITE);

    sprintf(buf, "DY=%d   ", g_last_dy);
    TFT_ShowString(185, 195, buf, WHITE);

    g_disp_dx = g_last_dx;
    g_disp_dy = g_last_dy;
}

/* =========================
   14b) 取当前用时 (ms)
   运行中：实时值；其余：冻结值
   ========================= */
static uint32_t Q2_GetElapsedMs(void)
{
    if(g_q2_state == Q2_RUNNING)
        return Tick_GetMs() - g_run_start_ms;
    return g_run_final_ms;
}

/* =========================
   14c) 计时刷新
   只在 0.1s 边界更新，避免
   每帧都写 TFT 占 SPI 时间
   ========================= */
static void Q2_UpdateTimer(void)
{
    uint32_t elapsed = Q2_GetElapsedMs();
    uint16_t sec10   = (uint16_t)(elapsed / 100);   /* 0.1s 单位 */
    uint16_t color;
    char     buf[16];

    if(sec10 == g_disp_sec10)
        return;   /* 不到 0.1s，跳过 */

    g_disp_sec10 = sec10;

    /* 颜色：绿(<20s) → 黄(20-25s) → 红(>25s)，压力感知 */
    if(elapsed < 20000)      color = GREEN;
    else if(elapsed < 25000) color = YELLOW;
    else                     color = RED;

    /* 格式："  12.3 s " —— 固定宽度防残留 */
    sprintf(buf, "  %2u.%u s ", (unsigned)(sec10 / 10), (unsigned)(sec10 % 10));

    /* 大数字行（在计时器区域中央） */
    TFT_ClearRect(CANVAS_X, CANVAS_Y + 92, CANVAS_W, 22, BLACK);
    TFT_ShowString(CANVAS_X + 16, CANVAS_Y + 96, buf, color);
}

/* =========================
   15) 文字显示（保持原样，
   只在全屏重绘时调用）
   ========================= */
static void Q2_ShowOneOption(uint16_t x, uint16_t y, const char *text, uint8_t selected, uint16_t color)
{
    char buf[20];

    if(selected)
        sprintf(buf, ">%s", text);
    else
        sprintf(buf, " %s", text);

    TFT_ShowString(x, y, buf, color);
}

static void Q2_ShowWords(uint16_t x, uint16_t y, const char *text, uint16_t color)
{
    char line[24];
    uint8_t i;
    uint16_t cy = y;

    while(*text)
    {
        while(*text == ' ')
            text++;

        if(*text == '\0')
            break;

        i = 0;
        while(*text != '\0' && *text != ' ' && i < sizeof(line) - 1)
            line[i++] = *text++;
        line[i] = '\0';

        TFT_ShowString(x, cy, line, color);
        cy += 22;
    }
}

static void Q2_ShowStateText(void)
{
    char buf[24];

    /* 注意：调用前已经TFT_Clear过，不需要ClearRect */
    TFT_ShowString(185, 120, "Q2", YELLOW);

    switch(g_q2_state)
    {
        case Q2_STOPPED:
            TFT_ShowString(185, 145, "STOP ", CYAN);   break;
        case Q2_PREPARE:
            TFT_ShowString(185, 145, "PREP ", CYAN);   break;
        case Q2_READY:
            TFT_ShowString(185, 145, "READY", GREEN);  break;
        case Q2_RUNNING:
            TFT_ShowString(185, 145, "RUN  ", RED);    break;
        case Q2_PAUSED:
            TFT_ShowString(185, 145, "PAUSE", BLUE);   break;
        case Q2_DONE:
            TFT_ShowString(185, 145, "DONE ", GREEN);  break;
        default:
            TFT_ShowString(185, 145, "UNK  ", RED);    break;
    }

    /* DX/DY先占位，由 Q2_UpdateDXDY 填写 */
    TFT_ShowString(185, 170, "DX=---  ", WHITE);
    TFT_ShowString(185, 195, "DY=---  ", WHITE);

    if(g_has_view)
    {
        if(g_view.rect_locked)
            TFT_ShowString(185, 220, "RECT:OK ", GREEN);
        else
            TFT_ShowString(185, 220, "RECT:-- ", YELLOW);

        if(g_view.laser_found)
            TFT_ShowString(185, 240, "LASER:OK", RED);
        else
            TFT_ShowString(185, 240, "LASER:--", YELLOW);
    }
    else
    {
        TFT_ShowString(185, 220, "RECT:-- ", YELLOW);
        TFT_ShowString(185, 240, "LASER:--", YELLOW);
    }
}

static void Q2_ShowHintText(void)
{
    TFT_ClearRect(185, 18, 135, 90, BLACK);

    if(g_q2_state == Q2_PREPARE)
    {
        Q2_ShowOneOption(205, 18, "START",  (g_q2_menu_sel == 0), YELLOW);
        Q2_ShowOneOption(205, 40, "RELOCK", (g_q2_menu_sel == 1), YELLOW);
        Q2_ShowOneOption(205, 62, "BACK",   (g_q2_menu_sel == 2), YELLOW);
    }
    else if(g_q2_state == Q2_READY)
    {
        Q2_ShowOneOption(205, 18, "START",  (g_q2_menu_sel == 0), GREEN);
        Q2_ShowOneOption(205, 40, "RELOCK", (g_q2_menu_sel == 1), GREEN);
        Q2_ShowOneOption(205, 62, "BACK",   (g_q2_menu_sel == 2), GREEN);
    }
    else if(g_q2_state == Q2_RUNNING)
    {
        Q2_ShowOneOption(205, 18, "PAUSE",  (g_q2_menu_sel == 0), YELLOW);
        Q2_ShowOneOption(205, 40, "DONE",   (g_q2_menu_sel == 1), YELLOW);
        Q2_ShowOneOption(205, 62, "BACK",   (g_q2_menu_sel == 2), YELLOW);
    }
    else if(g_q2_state == Q2_PAUSED)
    {
        Q2_ShowOneOption(205, 18, "RESUME", (g_q2_menu_sel == 0), YELLOW);
        Q2_ShowOneOption(205, 40, "DONE",   (g_q2_menu_sel == 1), YELLOW);
        Q2_ShowOneOption(205, 62, "BACK",   (g_q2_menu_sel == 2), YELLOW);
    }
    else if(g_q2_state == Q2_DONE)
    {
        Q2_ShowOneOption(205, 18, "BACK",   1, GREEN);
    }
    else
    {
        Q2_ShowOneOption(205, 18, "BACK",   1, YELLOW);
    }
}

/* =========================
   16) ROI坐标 -> 画布坐标
   ========================= */
static uint16_t Q2_MapX(int16_t x)
{
    int32_t num;

    if(x < Q2_SAFE_L) x = Q2_SAFE_L;
    if(x > Q2_SAFE_R) x = Q2_SAFE_R;

    num = (int32_t)(x - Q2_SAFE_L) * (CANVAS_W - 1);
    num /= (Q2_SAFE_R - Q2_SAFE_L);

    return (uint16_t)(CANVAS_X + num);
}

static uint16_t Q2_MapY(int16_t y)
{
    int32_t num;

    if(y < Q2_SAFE_T) y = Q2_SAFE_T;
    if(y > Q2_SAFE_B) y = Q2_SAFE_B;

    num = (int32_t)(y - Q2_SAFE_T) * (CANVAS_H - 1);
    num /= (Q2_SAFE_B - Q2_SAFE_T);

    return (uint16_t)(CANVAS_Y + num);
}

/* =========================
   17) 简单绘图
   ========================= */
static void Q2_DrawHLine(uint16_t x, uint16_t y, uint16_t w, uint16_t color)
{
    uint16_t i;
    for(i = 0; i < w; i++)
        TFT_FillBlock(x + i, y, 1, color);
}

static void Q2_DrawVLine(uint16_t x, uint16_t y, uint16_t h, uint16_t color)
{
    uint16_t i;
    for(i = 0; i < h; i++)
        TFT_FillBlock(x, y + i, 1, color);
}

static void Q2_DrawRect(uint16_t x, uint16_t y, uint16_t w, uint16_t h, uint16_t color)
{
    if(w < 2 || h < 2) return;

    Q2_DrawHLine(x, y, w, color);
    Q2_DrawHLine(x, y + h - 1, w, color);
    Q2_DrawVLine(x, y, h, color);
    Q2_DrawVLine(x + w - 1, y, h, color);
}

static void Q2_DrawPoint(uint16_t x, uint16_t y, uint8_t size, uint16_t color)
{
    TFT_FillBlock(x, y, size, color);
}

static void Q2_ClearViewData(void)
{
    memset(&g_view, 0, sizeof(g_view));
    g_has_view   = 0;
    g_rect_drawn = 0;
}
