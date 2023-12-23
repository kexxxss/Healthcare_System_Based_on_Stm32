#include "stm32f10x.h"
#include "key.h"
#include "sys.h" 
#include "delay.h"
  
								    
//������ʼ������
void KEY_Init(void) //IO��ʼ��
{ 
 	GPIO_InitTypeDef GPIO_InitStructure;
 
 	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOF,ENABLE);//ʹ��PORTFʱ��

	GPIO_InitStructure.GPIO_Pin  = GPIO_Pin_0|GPIO_Pin_1|GPIO_Pin_2;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IPU; //���ó���������
 	GPIO_Init(GPIOF, &GPIO_InitStructure);//��ʼ��GPIO

}


//�������������͵�ƽ��Ч
//���ذ���ֵ
//mode:0,��֧��������;1,֧��������;
//ע��˺�������Ӧ���ȼ�,KEY1>KEY2>KEY3����ΪC�Ķ�·����
u8 KEY_Scan(u8 mode)
{	 
	static u8 key_up=1;//�����ɿ���־
	if(mode)key_up=1;  //�Ƿ�֧������		  
	if(key_up&&(KEY1==0||KEY2==0||KEY3==0))
	{
		delay_ms(10);//ȥ���� 
		key_up=0;
		if(KEY1==0)return KEY_FINGER;
		else if(KEY2==0)return KEY_HEARTBLOOD;
		else if(KEY3==0)return KEY_TEMP;
	}else if(KEY1==1&&KEY2==1&&KEY3==1)key_up=1; 	    
 	return 0;// �ް�������
}
