#ifndef __KEY_H
#define __KEY_H	 
#include "sys.h"


#define KEY1  GPIO_ReadInputDataBit(GPIOF,GPIO_Pin_0)//读取按键1
#define KEY2  GPIO_ReadInputDataBit(GPIOF,GPIO_Pin_1)//读取按键2
#define KEY3  GPIO_ReadInputDataBit(GPIOF,GPIO_Pin_2)//读取按键3

#define KEY_FINGER 	1	    //控制指纹的按键按下
#define KEY_HEARTBLOOD 	2	//读取心率血氧数据的按键按下
#define KEY_TEMP 	3	    //读取体温的按键按下

void KEY_Init(void);//IO初始化
u8 KEY_Scan(u8);  	//按键扫描函数	

#endif
