#include "stm32f10x.h"                  // STM32F103标准库头文件
#include "Delay.h"                      // 毫秒延时函数（必须包含）
#include "Servo.h"                      // 舵机驱动头文件

int main(void)
{
	// 1. 初始化舵机（底层已包含PWM/TIM2/PA1初始化）
	Servo_Init();
	
	// 2. 上电复位：先归位到0度（初始位置）
	Servo_SetAngle(0.0f);  
	Delay_ms(1000);       // 延时1秒，确保舵机稳定复位到位
	
	// 3. 逐次转动1度，累计转45次（最终到45度）
	for(uint8_t i = 1; i <= 45; i++)
	{
		Servo_SetAngle((float)i);  // 每次角度+1度（1°→2°→...→45°）
		Delay_ms(80);              // 每次转动后短延时，适配舵机响应（可微调）
	}
	Delay_ms(1000);               // 45度位置停留1秒
	
	// 4. 最后回到90度
	Servo_SetAngle(90.0f);
	Delay_ms(1000);               // 确保舵机稳定转到90度
	
	// 死循环：最终停在90度位置
	while (1)
	{
		// 无额外动作，仅保持90度
	}
}