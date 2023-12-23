#ifndef __KEY_H
#define __KEY_H	 
#include "sys.h"


#define KEY1  GPIO_ReadInputDataBit(GPIOF,GPIO_Pin_0)//��ȡ����1
#define KEY2  GPIO_ReadInputDataBit(GPIOF,GPIO_Pin_1)//��ȡ����2
#define KEY3  GPIO_ReadInputDataBit(GPIOF,GPIO_Pin_2)//��ȡ����3

#define KEY_FINGER 	1	    //����ָ�Ƶİ�������
#define KEY_HEARTBLOOD 	2	//��ȡ����Ѫ�����ݵİ�������
#define KEY_TEMP 	3	    //��ȡ���µİ�������

void KEY_Init(void);//IO��ʼ��
u8 KEY_Scan(u8);  	//����ɨ�躯��	

#endif
