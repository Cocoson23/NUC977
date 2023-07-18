#include "nuc970.h"
#include "sys.h"
#include "gpio.h"

// 延时函数
void delay_ms(u32 i)
{
u32 j;
	for(;i>0;i--)
		for(j=50000;j>0;j--);
}

int main(void)
{
	  // 使用寄存器完成GPIO控制LED
	  if(0) {
			// 时钟CLK_PCLKEN0地址
			u32 CLK_PCLKEN0 = 0xB0000200 + 0x018;
			// GPIOPB方向寄存器地址
			u32 GPIOPB_DIR = 0xB8003000 + 0x040;  
			// GPIOPB数据输出寄存器地址
			u32 GPIOPB_DATAOUT = 0xB8003000 + 0x044;
		
			// 初始化GPIO时钟
			*((volatile unsigned int *)CLK_PCLKEN0) = 0x00000008;
			// 将PB0 PB1设置为输出
			*((volatile unsigned int *)GPIOPB_DIR) = 0x00000003;
			// 循环点亮
			while(1) {
				// PB0 ON PB1 OFF
				*((volatile unsigned int *)GPIOPB_DATAOUT) = 0x00000002;
				delay_ms(300);
				// PB0 OFF PB1 ON
				*((volatile unsigned int *)GPIOPB_DATAOUT) = 0x00000001;
				delay_ms(300);
			}
		}
		
		if(1) {
			  /**************************系统初始化开始**********************/
				*((volatile unsigned int *)REG_AIC_MDCR)=0xFFFFFFFF;
				*((volatile unsigned int *)REG_AIC_MDCRH)=0xFFFFFFFF;
				sysDisableCache();
				sysFlushCache(I_D_CACHE);
				sysEnableCache(CACHE_WRITE_BACK);
				/**************************系统初始化结束**********************/
			
				// 系统调试串口初始化 串口0 115200
				sysInitializeUART();

			  // 初始化PB0 PB1为推挽输出
				GPIO_OpenBit(GPIOB,BIT0, DIR_OUTPUT, PULL_UP);
				GPIO_OpenBit(GPIOB,BIT1, DIR_OUTPUT, PULL_UP);
				// 同时点亮 LED0 和 LED1
				GPIO_Clr(GPIOB, BIT0|BIT1);
				delay_ms(500);
				// 同时熄灭 LED0 和 LED1
				GPIO_Set(GPIOB,BIT0|BIT1);
				delay_ms(500);
				
				// LED0 LED1 交替闪烁
				while(1)
				{
					// PB0 ON PB1 OFF
					 GPIO_Clr(GPIOB, BIT0);
					 GPIO_Set(GPIOB, BIT1);
					 delay_ms(500);
					// PB0 OFF PB1 ON
					 GPIO_Clr(GPIOB, BIT1);
					 GPIO_Set(GPIOB, BIT0);
					 delay_ms(500);
				}
		}
}


