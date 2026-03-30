#include "Tick.h"

static volatile uint32_t g_ms = 0;

/* TIM2: 72MHz / 72 = 1MHz tick, ARR=999 -> 1ms interrupt */
void Tick_Init(void)
{
    TIM_TimeBaseInitTypeDef t;
    NVIC_InitTypeDef        n;

    RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM2, ENABLE);

    TIM_InternalClockConfig(TIM2);

    t.TIM_ClockDivision     = TIM_CKD_DIV1;
    t.TIM_CounterMode       = TIM_CounterMode_Up;
    t.TIM_Prescaler         = 72 - 1;    /* 1 MHz */
    t.TIM_Period            = 1000 - 1;  /* 1 ms  */
    t.TIM_RepetitionCounter = 0;
    TIM_TimeBaseInit(TIM2, &t);

    TIM_ITConfig(TIM2, TIM_IT_Update, ENABLE);
    TIM_Cmd(TIM2, ENABLE);

    /* 中断优先级：比 UART 低，但比空闲任务高 */
    n.NVIC_IRQChannel                   = TIM2_IRQn;
    n.NVIC_IRQChannelPreemptionPriority = 1;
    n.NVIC_IRQChannelSubPriority        = 0;
    n.NVIC_IRQChannelCmd                = ENABLE;
    NVIC_Init(&n);
}

uint32_t Tick_GetMs(void)
{
    return g_ms;
}

void TIM2_IRQHandler(void)
{
    if(TIM_GetITStatus(TIM2, TIM_IT_Update) != RESET)
    {
        g_ms++;
        TIM_ClearITPendingBit(TIM2, TIM_IT_Update);
    }
}
