/*************************************************************************************
������ߣ�
STM32F103-MAX30102:
	VCC<->3.3V
	GND<->GND
	SCL<->PB6
	SDA<->PB7
	IM(INT)<->PB9
0.96inch OLED :
      GND    ��Դ��
      VCC  ��3.3v��Դ
      D0   ��PG12��SCL��
      D1   ��PD5��SDA��
      RES  ��PD4
      DC   ��PD15
      CS   ��PD1  
����1 - ���Դ��ڣ����İ������Ӻã�:
	5V<->5V
	GND<->GND
	RX<->PA9
	TX<->PA10
����2 - ESP8266:
	3.3V<->3.3V
	GND<->GND
	RX<->PA2
	TX<->PA3
    RST<->PA4
����4 - AS608ָ��ģ��:
	VCC<->3.3v(�м�ֻ�ܽ�3.3v��
	GND<->GND
	RX<->PC10
	TX<->PC11
IIC2 - GY-906��MLX90614���������
	VCC<->3.3v
	GND<->GND
	SCL<->PB10
	SDA<->PB11
**************************************************************************************/
//MCUͷ�ļ�
#include "stm32f10x.h"

//Ӳ������
#include "sys.h"
#include "usart.h"
#include "led.h"
#include "key.h"
#include "delay.h"
#include "myiic.h"
#include "IIC2.h"
#include "oled.h"
#include "timer.h"
#include "as608.h"
#include "max30102.h" 

//����Ѫ���������Ĳ����㷨
#include "algorithm.h"

//����Э���
#include "onenet.h"

//�����豸
#include "esp8266.h"

//C��

#define DEVICE_ID 567   //�豸id��ÿ̨�豸id��Ψһ�ԣ����Ӧ��ͨ��һ���豸���ݿ��ڲ�Ʒ������ʱ������
                     //��ô�Ҹ��˿�����ʡ������id��һ�����ˣ�--��������̨�豸���ʽ�û��--��emo...
void get_temperature(void);//��ȡ����
void get_heartblood(void);//��ȡ����Ѫ��

u16 deviceid=DEVICE_ID; //�豸idȫ�ֱ��������������ļ���
int user_flag;//���������豸�Ƿ��ѵ�¼�û���Ϊ1��ʾ��¼���ϴ�����
const char *Topics[] = {"/xm/health","/xm/addMember"}; //mqtt���⣬�����շ��û�����

//max30102����
uint32_t aun_ir_buffer[500]; //IR LED sensor data �������ݣ����ڼ���Ѫ��
int32_t n_ir_buffer_length  = 500;    //data length
uint32_t aun_red_buffer[500];    //Red LED sensor data ������ݣ����ڼ������������Լ���������
int32_t n_sp02;         //Ѫ��ֵ
int8_t ch_spo2_valid;   //Ѫ��ֵ�����Ч�ԣ�0��Ч��1��Ч
int32_t n_heart_rate;   //����ֵ
int8_t  ch_hr_valid;    //����ֵ�����Ч�ԣ�0��Ч��1��Ч

//�û�����
typedef struct
{
    u16 memberfid;           //ָ��ID
    u8 heart;          //����
    u8 blood;    //Ѫ��ֵ
    float temperature;      //����
}USER;

USER usr;
char buf[128];

//Ӳ����ʼ��
void Hardware_Init(void)
{
    NVIC_Configuration();   //�����ж����ȼ�����
	delay_init();	    	 //��ʱ������ʼ��	  
	Usart1_Init(115200);	 //���Դ��ڣ������ʳ�ʼ��Ϊ115200
    Usart2_Init(115200);     //��esp8266ͨѶʹ�õĴ���
    Usart4_Init(57600);
    ESP8266_Init();			//��ʼ��ESP8266
    KEY_Init();         //������ʼ��
	OLED_Init();        //OLed��ʼ��
    SMBus_Init();   //����ģ��ͨ�����ų�ʼ��
    max30102_init();          //����Ѫ����������ʼ��
    //TIM2_Int_Init(4999,7199); //��ʱ��2��ʼ������ʱʱ��500ms
}



int main(void)
{ 
    u8 keyval = 0;//������ֵ
    int temp;//��ʱ��
    unsigned char *dataPtr = NULL;//���ڱ����mqtt���������յ�����Ϣ
    
    user_flag = 0;//��ʼ��Ϊ�û�δ��¼
    Hardware_Init();//Ӳ����ʼ����MCU������
    OLED_Show_Await_Face();//��ʾ�����棨����ģʽ��
  
	while(OneNet_DevLink())			//����Mqtt������
	delay_ms(500);
    OneNet_Subscribe(Topics, 2);  //��������
      
    delay_ms(10);
    while(1)
	{
 		keyval=KEY_Scan(0);	//�õ���ֵ
	   	if(keyval)
		{						   
			switch(keyval)
			{				 
				case KEY_FINGER:
                    temp = press_FR();
                    UsartPrintf(USART_DEBUG,"key1 press,fid =%d",temp);
                    if(temp!= -1)
                    {
                        usr.memberfid=temp;
                    }                    					
                    OLED_Show_Await_Face();
					break;
                
				case KEY_HEARTBLOOD:	
					
                    UsartPrintf(USART_DEBUG,"key2 press");
                    get_heartblood();
                    OLED_Show_Await_Face();
					break;
                
				case KEY_TEMP:	
					UsartPrintf(USART_DEBUG,"key3 press");
                    get_temperature();
                    OLED_Show_Await_Face();
					break;                
			}
		}
    /*����mqtt����*/
        dataPtr = ESP8266_GetIPD(4); //�ȴ�ָ��������ʱ��5*10ms
        if(dataPtr != NULL)
            OneNet_RevPro(dataPtr);        
	}
}


//��ȡ����Ѫ��
void get_heartblood(void)
{
 	int i;
	u8 temp[6];
    //�洢������Ч����ֵ
    int32_t heart[10];
    int32_t sp02[10]; 
    //��ʱ��
    int32_t sumheart,sumsp02;
  	u8 count = 0;
    //oled��ʾ
    OLED_Clear();
    OLED_ShowChinese(0,0,0,16);//Զ
    OLED_ShowChinese(16,0,1,16);//��
    OLED_ShowChinese(32,0,2,16);//ҽ
    OLED_ShowChinese(48,0,3,16);//��
    OLED_ShowChinese(64,0,4,16);//��
    OLED_ShowChinese(80,0,5,16);//��
    OLED_ShowChinese(96,0,6,16);//ϵ
    OLED_ShowChinese(112,0,7,16);//ͳ
    
    OLED_ShowChinese(32,32,39,16);//��
    OLED_ShowChinese(48,32,40,16);//��
    OLED_ShowChinese(64,32,41,16);//��
    OLED_ShowChar(82,32,'.',16);
    OLED_ShowChar(86,32,'.',16);  
    OLED_ShowChar(90,32,'.',16); 
    OLED_Refresh();
	while(1)
	{
        //����ֵ�����㷨���û����500�����ݷ�����ÿ�δӴ�������ȡ���µ�100��������Ȼ���ٷ���
		//�Ƴ�ǰһ�ٸ����ݣ�������������ǰŲ100
        for(i=100;i<500;i++)
        {
            aun_red_buffer[i-100]=aun_red_buffer[i];
            aun_ir_buffer[i-100]=aun_ir_buffer[i];           
        }
        //�Ӵ�������ȡһ�ٸ����ݣ����ڻ���400-500����λ���ϣ����»���
        for(i=400;i<500;i++)
        {
            while(MAX30102_INT==1);//�ȴ�ֱ���ж����Ŷ���
            max30102_FIFO_ReadBytes(REG_FIFO_DATA,temp);//��ȡ����������
            //��ֵ�ϲ��õ�ʵ�����֣�����400-500Ϊ�¶�ȡ����
			aun_red_buffer[i] =  (long)((long)((long)temp[0]&0x03)<<16) | (long)temp[1]<<8 | (long)temp[2];    
			aun_ir_buffer[i] = (long)((long)((long)temp[3] & 0x03)<<16) |(long)temp[4]<<8 | (long)temp[5];         
		}
        //����500�����ʺ�Ѫ�����ݣ���������ʺ�Ѫ��ֵ
        maxim_heart_rate_and_oxygen_saturation(aun_ir_buffer, n_ir_buffer_length, aun_red_buffer, 
        &n_sp02, &ch_spo2_valid, &n_heart_rate, &ch_hr_valid);
        //�ж��Ƿ���Ч�ҷ��ϼ�ⷶΧ
        if(ch_hr_valid == 1 && ch_spo2_valid ==1 && n_heart_rate<200 && n_sp02<101){
            //���ڵ������
            UsartPrintf(USART_DEBUG,"HR=%i, ", n_heart_rate); 
            UsartPrintf(USART_DEBUG,"HRvalid=%i, ", ch_hr_valid);
            UsartPrintf(USART_DEBUG,"SpO2=%i, ", n_sp02);
            UsartPrintf(USART_DEBUG,"SPO2Valid=%i\r\n", ch_spo2_valid);
            //������ֵ
            heart[count]=n_heart_rate;
            sp02[count] =n_sp02;
            count++;
            //������������ƽ��ֵ��led��ʾ�����
            if(count==5){
                count = 0;
                sumheart = 0;
                sumsp02 = 0;
                for(i=0;i<5;i++){
                    sumheart += heart[i];
                    sumsp02 += sp02[i];
                }
                usr.heart = sumheart/5;
                usr.blood = sumsp02/5;
                UsartPrintf(USART_DEBUG,"sumh:%d,sumsp02:%d,heart= %d ,sp02=%d\n",sumheart,sumsp02,usr.heart,usr.blood);
                OLED_Clear();
                OLED_ShowChinese(0,0,0,16);//Զ
                OLED_ShowChinese(16,0,1,16);//��
                OLED_ShowChinese(32,0,2,16);//ҽ
                OLED_ShowChinese(48,0,3,16);//��
                OLED_ShowChinese(64,0,4,16);//��
                OLED_ShowChinese(80,0,5,16);//��
                OLED_ShowChinese(96,0,6,16);//ϵ
                OLED_ShowChinese(112,0,7,16);//ͳ
                
                OLED_ShowChinese(0,16,42,16);//��
                OLED_ShowChinese(16,16,43,16);//��
                OLED_ShowChar(34,16,':',16);//:
                if(usr.heart<100){
                    OLED_ShowNum(40,16,usr.heart,2,16);
                }else{
                    OLED_ShowNum(40,16,usr.heart,3,16);
                }
                OLED_ShowChinese(0,32,44,16);//Ѫ
                OLED_ShowChinese(16,32,45,16);//��
                OLED_ShowChar(34,32,':',16);//:
                if(usr.blood<100){
                    OLED_ShowNum(40,32,usr.blood,2,16);
                }else{
                    OLED_ShowNum(40,32,usr.blood,3,16);
                }
                OLED_Refresh();
                if(user_flag==1){
                    //�û��ѵ�¼���ϴ�����
                    /* mqtt ���ķ���*/
                    UsartPrintf(USART_DEBUG, "OneNet_Publish\r\n");
                    sprintf(
                            buf,
                    "{\"device\":%d,\"memberfid\":%d,\"heart\":%d,\"blood\":%d}",
                            DEVICE_ID,usr.memberfid,usr.heart,usr.blood);
                    OneNet_Publish(*Topics, buf);  
                    ESP8266_Clear();
                    
                    OLED_ShowChar(0,48,'(',16);
                    OLED_ShowChinese(16,48,46,16);//��
                    OLED_ShowChinese(32,48,47,16);//��
                    OLED_ShowChinese(48,48,48,16);//��
                    OLED_ShowChinese(64,48,49,16);//��
                    OLED_ShowChinese(80,48,50,16);//��
                    OLED_ShowChar(100,48,')',16);
                    OLED_Refresh();
                    
                }               
                break;
            }
        }		       
	}
    //��ʱ�������1864����λms
    delay_ms(1700);
    delay_ms(1700);
    delay_ms(1700);
}
//�õ���������
void get_temperature()
{   
    float temp;
    usr.temperature = SMBus_ReadTemp();
    UsartPrintf(USART_DEBUG,"temperature= %.1f",usr.temperature);
    
    temp = usr.temperature + 0.05;//��Ϊoled��������Զ��������룬�����ֶ����еڶ�λС������������
    OLED_Clear();
    OLED_ShowChinese(0,0,0,16);//Զ
    OLED_ShowChinese(16,0,1,16);//��
    OLED_ShowChinese(32,0,2,16);//ҽ
    OLED_ShowChinese(48,0,3,16);//��
    OLED_ShowChinese(64,0,4,16);//��
    OLED_ShowChinese(80,0,5,16);//��
    OLED_ShowChinese(96,0,6,16);//ϵ
    OLED_ShowChinese(112,0,7,16);//ͳ
    OLED_ShowChinese(0,32,36,16);//��
    OLED_ShowChinese(16,32,37,16);//��
    OLED_ShowChar(34,32,':',16);//:
    OLED_ShowNum(42,32,((int)temp)/10,1,16); //ʮλ
    OLED_ShowNum(52,32,((int)temp)%10,1,16); //��λ
    OLED_ShowChar(60,32,'.',16);//.С����
    OLED_ShowNum(65,32,((int)(temp*10))%10,1,16); //С��λ
    OLED_ShowChinese(76,32,38,16);//��
    OLED_Refresh();
    
    if(user_flag==1)
    {
        //�û��ѵ�¼���ϴ�����
        /* mqtt ���ķ���*/
        UsartPrintf(USART_DEBUG, "OneNet_Publish\r\n");
        sprintf(
                buf,
                "{\"device\":%d,\"memberfid\":%d,\"temperature\":%.1f}",
                DEVICE_ID,usr.memberfid,usr.temperature);
        OneNet_Publish(*Topics, buf);  
        ESP8266_Clear();
        
        OLED_ShowChar(0,48,'(',16);
        OLED_ShowChinese(16,48,46,16);//��
        OLED_ShowChinese(32,48,47,16);//��
        OLED_ShowChinese(48,48,48,16);//��
        OLED_ShowChinese(64,48,49,16);//��
        OLED_ShowChinese(80,48,50,16);//��
        OLED_ShowChar(100,48,')',16);
        OLED_Refresh();
        
    }
    //��ʱ�������1864
    delay_ms(1700);
    delay_ms(1700);
    delay_ms(1700);
}
