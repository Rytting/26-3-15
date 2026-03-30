#include "stm32f10x.h"
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include "K230_UART.h"

#define RX_BUF_LEN  128

static char    RxBuf[RX_BUF_LEN];
static uint8_t RxIndex = 0;
static uint8_t RxStart = 0;

/* =========================
   已注册的回调
   ========================= */
static K230_DiffCallback     g_diff_cb    = 0;
static K230_Q2ViewCallback   g_q2view_cb  = 0;
static K230_AutoDoneCallback g_done_cb    = 0;

void K230_RegisterCallbacks(K230_DiffCallback     diff_cb,
                             K230_Q2ViewCallback   q2view_cb,
                             K230_AutoDoneCallback done_cb)
{
    g_diff_cb   = diff_cb;
    g_q2view_cb = q2view_cb;
    g_done_cb   = done_cb;
}

/* =========================
   帧解析（内部）
   ========================= */
static void K230_ParseFrame(char *buf);
static void K230_ParseDifFrame(char *buf);
static void K230_ParseQ2VFrame(char *buf);

static void K230_ParseDifFrame(char *buf)
{
    char   *token;
    int     field = 0;
    int16_t dx = 0, dy = 0;

    token = strtok(buf + 5, ",*");   /* 跳过 "$DIF," */
    while(token != NULL)
    {
        if(field == 0) dx = (int16_t)atoi(token);
        else if(field == 1)
        {
            dy = (int16_t)atoi(token);
            if(g_diff_cb) g_diff_cb(dx, dy);
            return;
        }
        field++;
        token = strtok(NULL, ",*");
    }
}

static void K230_ParseQ2VFrame(char *buf)
{
    char   *token;
    int     field = 0;
    int16_t xl=0, xr=0, yt=0, yb=0, tx=0, ty=0, bx=0, by=0;
    uint8_t flags = 0;

    token = strtok(buf + 5, ",*");   /* 跳过 "$Q2V," */
    while(token != NULL)
    {
        switch(field)
        {
            case 0: xl    = (int16_t)atoi(token); break;
            case 1: xr    = (int16_t)atoi(token); break;
            case 2: yt    = (int16_t)atoi(token); break;
            case 3: yb    = (int16_t)atoi(token); break;
            case 4: tx    = (int16_t)atoi(token); break;
            case 5: ty    = (int16_t)atoi(token); break;
            case 6: bx    = (int16_t)atoi(token); break;
            case 7: by    = (int16_t)atoi(token); break;
            case 8:
                flags = (uint8_t)atoi(token);
                if(g_q2view_cb) g_q2view_cb(xl, xr, yt, yb, tx, ty, bx, by, flags);
                return;
            default: return;
        }
        field++;
        token = strtok(NULL, ",*");
    }
}

static void K230_ParseFrame(char *buf)
{
    char temp[RX_BUF_LEN];
    strncpy(temp, buf, RX_BUF_LEN - 1);
    temp[RX_BUF_LEN - 1] = '\0';

    if(strncmp(temp, "$DIF,", 5) == 0)
    {
        K230_ParseDifFrame(temp);
    }
    else if(strncmp(temp, "$Q2V,", 5) == 0)
    {
        K230_ParseQ2VFrame(temp);
    }
    else if(strncmp(temp, "$STATE,DONE*", 12) == 0)
    {
        /* 中断里只置标志，实际处理交给注册的回调（应为 AutoDoneFromISR） */
        if(g_done_cb) g_done_cb();
    }
    /* 其他 $STATE,... / $ACK,... 帧暂忽略 */
}

/* =========================
   UART 初始化
   ========================= */
void K230_Uart_Init(void)
{
    GPIO_InitTypeDef   GPIO_InitStructure;
    USART_InitTypeDef  USART_InitStructure;
    NVIC_InitTypeDef   NVIC_InitStructure;

    RCC_APB2PeriphClockCmd(RCC_APB2Periph_USART1, ENABLE);
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA,  ENABLE);

    /* PA9 -> TX */
    GPIO_InitStructure.GPIO_Mode  = GPIO_Mode_AF_PP;
    GPIO_InitStructure.GPIO_Pin   = GPIO_Pin_9;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_Init(GPIOA, &GPIO_InitStructure);

    /* PA10 -> RX */
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IPU;
    GPIO_InitStructure.GPIO_Pin  = GPIO_Pin_10;
    GPIO_Init(GPIOA, &GPIO_InitStructure);

    USART_InitStructure.USART_BaudRate            = 115200;
    USART_InitStructure.USART_HardwareFlowControl = USART_HardwareFlowControl_None;
    USART_InitStructure.USART_Mode                = USART_Mode_Rx | USART_Mode_Tx;
    USART_InitStructure.USART_Parity              = USART_Parity_No;
    USART_InitStructure.USART_StopBits            = USART_StopBits_1;
    USART_InitStructure.USART_WordLength           = USART_WordLength_8b;
    USART_Init(USART1, &USART_InitStructure);

    USART_ITConfig(USART1, USART_IT_RXNE, ENABLE);

    NVIC_PriorityGroupConfig(NVIC_PriorityGroup_2);
    NVIC_InitStructure.NVIC_IRQChannel                   = USART1_IRQn;
    NVIC_InitStructure.NVIC_IRQChannelCmd                = ENABLE;
    NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 1;
    NVIC_InitStructure.NVIC_IRQChannelSubPriority        = 1;
    NVIC_Init(&NVIC_InitStructure);

    USART_Cmd(USART1, ENABLE);
}

/* =========================
   发送接口
   ========================= */
void K230_Uart_SendString(char *str)
{
    while(*str)
    {
        USART_SendData(USART1, *str++);
        while(USART_GetFlagStatus(USART1, USART_FLAG_TXE) == RESET);
    }
}

void K230_SetMode(uint8_t mode)
{
    char buf[32];
    sprintf(buf, "$MODE,%d*\r\n", mode);
    K230_Uart_SendString(buf);
}

void K230_Run(uint8_t en)
{
    char buf[32];
    sprintf(buf, "$RUN,%d*\r\n", en ? 1 : 0);
    K230_Uart_SendString(buf);
}

void K230_Stop(void)
{
    K230_Uart_SendString("$STOP,1*\r\n");
}

void K230_ResetVision(void)
{
    K230_Uart_SendString("$RST,1*\r\n");
}

/* =========================
   中断服务
   ========================= */
void USART1_IRQHandler(void)
{
    uint8_t ch;

    if(USART_GetITStatus(USART1, USART_IT_RXNE) == SET)
    {
        ch = (uint8_t)USART_ReceiveData(USART1);

        if(ch == '$')
        {
            RxStart = 1;
            RxIndex = 0;
            RxBuf[RxIndex++] = ch;
        }
        else if(RxStart)
        {
            if(RxIndex < RX_BUF_LEN - 1)
                RxBuf[RxIndex++] = ch;

            if(ch == '*')
            {
                RxBuf[RxIndex] = '\0';
                RxStart = 0;
                K230_ParseFrame(RxBuf);
            }
        }

        USART_ClearITPendingBit(USART1, USART_IT_RXNE);
    }
}
