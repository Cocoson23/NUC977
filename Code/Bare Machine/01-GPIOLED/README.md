# GPIO LED #
使用GPIO控制LED灯亮灭  
- 可直接通过指针对寄存器进行设置
- 可通过提供的BSP完成
## Steps ##
1. 根据原理图观察`User LED`对应的GPIO为PB0与PB1
2. 在`NUC970_TRM.pdf`中寻找GPIOPB对应的寄存器  
   - `GPIOB_DIR` GPIO Port B Direction Control Register  
   GPIO_BA+0x040  
   - `GPIOB_DATAOUT`  GPIO Port B Data Output Register  
   GPIO_BA+0x044  
   - `CLK_PCLKEN0` APB Devices Clock Enable Control Register 0  
   CLK_BA+0x018
3. 初始化时钟激活GPIO
4. 将PB0 PB1设置为推挽输出
5. 根据需求设置`GPIOB_DATAOUT`第0、1位高低电平以实现LED灯亮灭变化
6. build并于USB mode烧录
7. 于NAND Boot模式RST
![GPIO LED](https://img1.imgtp.com/2023/07/18/e9INL1Fr.JPG)