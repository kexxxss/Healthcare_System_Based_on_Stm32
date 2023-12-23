#include "stm32f10x.h"
#include "key.h"
#include "sys.h" 
#include "delay.h"
  
								    
//按键初始化函数
void KEY_Init(void) //IO初始化
{ 
 	GPIO_InitTypeDef GPIO_InitStructure;
 
 	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOF,ENABLE);//使能PORTF时钟

	GPIO_InitStructure.GPIO_Pin  = GPIO_Pin_0|GPIO_Pin_1|GPIO_Pin_2;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IPU; //设置成上拉输入
 	GPIO_Init(GPIOF, &GPIO_InitStructure);//初始化GPIO

}


//按键处理函数，低电平有效
//返回按键值
//mode:0,不支持连续按;1,支持连续按;
//注意此函数有响应优先级,KEY1>KEY2>KEY3，因为C的短路法则
u8 KEY_Scan(u8 mode)
{	 
	static u8 key_up=1;//按键松开标志
	if(mode)key_up=1;  //是否支持连按		  
	if(key_up&&(KEY1==0||KEY2==0||KEY3==0))
	{
		delay_ms(10);//去抖动 
		key_up=0;
		if(KEY1==0)return KEY_FINGER;
		else if(KEY2==0)return KEY_HEARTBLOOD;
		else if(KEY3==0)return KEY_TEMP;
	}else if(KEY1==1&&KEY2==1&&KEY3==1)key_up=1; 	    
 	return 0;// 无按键按下
}
