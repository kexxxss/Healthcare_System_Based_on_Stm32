/*************************************************************************************
外设接线：
STM32F103-MAX30102:
	VCC<->3.3V
	GND<->GND
	SCL<->PB6
	SDA<->PB7
	IM(INT)<->PB9
0.96inch OLED :
      GND    电源地
      VCC  接3.3v电源
      D0   接PG12（SCL）
      D1   接PD5（SDA）
      RES  接PD4
      DC   接PD15
      CS   接PD1  
串口1 - 调试串口（核心板已连接好）:
	5V<->5V
	GND<->GND
	RX<->PA9
	TX<->PA10
串口2 - ESP8266:
	3.3V<->3.3V
	GND<->GND
	RX<->PA2
	TX<->PA3
    RST<->PA4
串口4 - AS608指纹模块:
	VCC<->3.3v(切记只能接3.3v）
	GND<->GND
	RX<->PC10
	TX<->PC11
IIC2 - GY-906（MLX90614）红外测温
	VCC<->3.3v
	GND<->GND
	SCL<->PB10
	SDA<->PB11
**************************************************************************************/
//MCU头文件
#include "stm32f10x.h"

//硬件驱动
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

//心率血氧传感器的补偿算法
#include "algorithm.h"

//网络协议层
#include "onenet.h"

//网络设备
#include "esp8266.h"

//C库

#define DEVICE_ID 567   //设备id，每台设备id具唯一性，这个应该通过一个设备数据库在产品出厂的时候设置
                     //那么我个人开发就省掉生成id这一部分了，--我连做五台设备的资金都没有--，emo...
void get_temperature(void);//获取体温
void get_heartblood(void);//获取心率血氧

u16 deviceid=DEVICE_ID; //设备id全局变量，方便其他文件用
int user_flag;//用于区分设备是否已登录用户，为1表示登录则上传数据
const char *Topics[] = {"/xm/health","/xm/addMember"}; //mqtt主题，用于收发用户数据

//max30102参数
uint32_t aun_ir_buffer[500]; //IR LED sensor data 红外数据，用于计算血氧
int32_t n_ir_buffer_length  = 500;    //data length
uint32_t aun_red_buffer[500];    //Red LED sensor data 红光数据，用于计算心率曲线以及计算心率
int32_t n_sp02;         //血氧值
int8_t ch_spo2_valid;   //血氧值检测有效性，0无效，1有效
int32_t n_heart_rate;   //心率值
int8_t  ch_hr_valid;    //心率值检测有效性，0无效，1有效

//用户数据
typedef struct
{
    u16 memberfid;           //指纹ID
    u8 heart;          //心率
    u8 blood;    //血氧值
    float temperature;      //体温
}USER;

USER usr;
char buf[128];

//硬件初始化
void Hardware_Init(void)
{
    NVIC_Configuration();   //设置中断优先级分组
	delay_init();	    	 //延时函数初始化	  
	Usart1_Init(115200);	 //调试串口，波特率初始化为115200
    Usart2_Init(115200);     //与esp8266通讯使用的串口
    Usart4_Init(57600);
    ESP8266_Init();			//初始化ESP8266
    KEY_Init();         //按键初始化
	OLED_Init();        //OLed初始化
    SMBus_Init();   //测温模块通信引脚初始化
    max30102_init();          //心率血氧传感器初始化
    //TIM2_Int_Init(4999,7199); //定时器2初始化，定时时间500ms
}



int main(void)
{ 
    u8 keyval = 0;//按键键值
    int temp;//临时量
    unsigned char *dataPtr = NULL;//用于保存从mqtt服务器接收到的消息
    
    user_flag = 0;//初始化为用户未登录
    Hardware_Init();//硬件初始化，MCU和外设
    OLED_Show_Await_Face();//显示主界面（待机模式）
  
	while(OneNet_DevLink())			//接入Mqtt服务器
	delay_ms(500);
    OneNet_Subscribe(Topics, 2);  //订阅主题
      
    delay_ms(10);
    while(1)
	{
 		keyval=KEY_Scan(0);	//得到键值
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
    /*接收mqtt报文*/
        dataPtr = ESP8266_GetIPD(4); //等待指令消化的时间5*10ms
        if(dataPtr != NULL)
            OneNet_RevPro(dataPtr);        
	}
}


//获取心率血氧
void get_heartblood(void)
{
 	int i;
	u8 temp[6];
    //存储检测的有效数据值
    int32_t heart[10];
    int32_t sp02[10]; 
    //临时量
    int32_t sumheart,sumsp02;
  	u8 count = 0;
    //oled显示
    OLED_Clear();
    OLED_ShowChinese(0,0,0,16);//远
    OLED_ShowChinese(16,0,1,16);//程
    OLED_ShowChinese(32,0,2,16);//医
    OLED_ShowChinese(48,0,3,16);//疗
    OLED_ShowChinese(64,0,4,16);//保
    OLED_ShowChinese(80,0,5,16);//健
    OLED_ShowChinese(96,0,6,16);//系
    OLED_ShowChinese(112,0,7,16);//统
    
    OLED_ShowChinese(32,32,39,16);//测
    OLED_ShowChinese(48,32,40,16);//量
    OLED_ShowChinese(64,32,41,16);//中
    OLED_ShowChar(82,32,'.',16);
    OLED_ShowChar(86,32,'.',16);  
    OLED_ShowChar(90,32,'.',16); 
    OLED_Refresh();
	while(1)
	{
        //生理值计算算法采用缓存的500组数据分析，每次从传感器获取最新的100组新数据然后再分析
		//移除前一百个数据，即所有数据往前挪100
        for(i=100;i<500;i++)
        {
            aun_red_buffer[i-100]=aun_red_buffer[i];
            aun_ir_buffer[i-100]=aun_ir_buffer[i];           
        }
        //从传感器读取一百个数据，放在缓存400-500数组位置上，更新缓存
        for(i=400;i<500;i++)
        {
            while(MAX30102_INT==1);//等待直到中断引脚断言
            max30102_FIFO_ReadBytes(REG_FIFO_DATA,temp);//读取传感器数据
            //将值合并得到实际数字，数组400-500为新读取数据
			aun_red_buffer[i] =  (long)((long)((long)temp[0]&0x03)<<16) | (long)temp[1]<<8 | (long)temp[2];    
			aun_ir_buffer[i] = (long)((long)((long)temp[3] & 0x03)<<16) |(long)temp[4]<<8 | (long)temp[5];         
		}
        //传入500个心率和血氧数据，计算出心率和血氧值
        maxim_heart_rate_and_oxygen_saturation(aun_ir_buffer, n_ir_buffer_length, aun_red_buffer, 
        &n_sp02, &ch_spo2_valid, &n_heart_rate, &ch_hr_valid);
        //判断是否有效且符合检测范围
        if(ch_hr_valid == 1 && ch_spo2_valid ==1 && n_heart_rate<200 && n_sp02<101){
            //串口调试输出
            UsartPrintf(USART_DEBUG,"HR=%i, ", n_heart_rate); 
            UsartPrintf(USART_DEBUG,"HRvalid=%i, ", ch_hr_valid);
            UsartPrintf(USART_DEBUG,"SpO2=%i, ", n_sp02);
            UsartPrintf(USART_DEBUG,"SPO2Valid=%i\r\n", ch_spo2_valid);
            //存入检测值
            heart[count]=n_heart_rate;
            sp02[count] =n_sp02;
            count++;
            //如果存满则计算平均值并led显示和输出
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
                OLED_ShowChinese(0,0,0,16);//远
                OLED_ShowChinese(16,0,1,16);//程
                OLED_ShowChinese(32,0,2,16);//医
                OLED_ShowChinese(48,0,3,16);//疗
                OLED_ShowChinese(64,0,4,16);//保
                OLED_ShowChinese(80,0,5,16);//健
                OLED_ShowChinese(96,0,6,16);//系
                OLED_ShowChinese(112,0,7,16);//统
                
                OLED_ShowChinese(0,16,42,16);//心
                OLED_ShowChinese(16,16,43,16);//率
                OLED_ShowChar(34,16,':',16);//:
                if(usr.heart<100){
                    OLED_ShowNum(40,16,usr.heart,2,16);
                }else{
                    OLED_ShowNum(40,16,usr.heart,3,16);
                }
                OLED_ShowChinese(0,32,44,16);//血
                OLED_ShowChinese(16,32,45,16);//氧
                OLED_ShowChar(34,32,':',16);//:
                if(usr.blood<100){
                    OLED_ShowNum(40,32,usr.blood,2,16);
                }else{
                    OLED_ShowNum(40,32,usr.blood,3,16);
                }
                OLED_Refresh();
                if(user_flag==1){
                    //用户已登录，上传数据
                    /* mqtt 报文发送*/
                    UsartPrintf(USART_DEBUG, "OneNet_Publish\r\n");
                    sprintf(
                            buf,
                    "{\"device\":%d,\"memberfid\":%d,\"heart\":%d,\"blood\":%d}",
                            DEVICE_ID,usr.memberfid,usr.heart,usr.blood);
                    OneNet_Publish(*Topics, buf);  
                    ESP8266_Clear();
                    
                    OLED_ShowChar(0,48,'(',16);
                    OLED_ShowChinese(16,48,46,16);//数
                    OLED_ShowChinese(32,48,47,16);//据
                    OLED_ShowChinese(48,48,48,16);//已
                    OLED_ShowChinese(64,48,49,16);//上
                    OLED_ShowChinese(80,48,50,16);//传
                    OLED_ShowChar(100,48,')',16);
                    OLED_Refresh();
                    
                }               
                break;
            }
        }		       
	}
    //延时参数最大1864，单位ms
    delay_ms(1700);
    delay_ms(1700);
    delay_ms(1700);
}
//得到体温数据
void get_temperature()
{   
    float temp;
    usr.temperature = SMBus_ReadTemp();
    UsartPrintf(USART_DEBUG,"temperature= %.1f",usr.temperature);
    
    temp = usr.temperature + 0.05;//因为oled输出不能自动四舍五入，这里手动进行第二位小数的四舍五入
    OLED_Clear();
    OLED_ShowChinese(0,0,0,16);//远
    OLED_ShowChinese(16,0,1,16);//程
    OLED_ShowChinese(32,0,2,16);//医
    OLED_ShowChinese(48,0,3,16);//疗
    OLED_ShowChinese(64,0,4,16);//保
    OLED_ShowChinese(80,0,5,16);//健
    OLED_ShowChinese(96,0,6,16);//系
    OLED_ShowChinese(112,0,7,16);//统
    OLED_ShowChinese(0,32,36,16);//体
    OLED_ShowChinese(16,32,37,16);//温
    OLED_ShowChar(34,32,':',16);//:
    OLED_ShowNum(42,32,((int)temp)/10,1,16); //十位
    OLED_ShowNum(52,32,((int)temp)%10,1,16); //个位
    OLED_ShowChar(60,32,'.',16);//.小数点
    OLED_ShowNum(65,32,((int)(temp*10))%10,1,16); //小数位
    OLED_ShowChinese(76,32,38,16);//度
    OLED_Refresh();
    
    if(user_flag==1)
    {
        //用户已登录，上传数据
        /* mqtt 报文发送*/
        UsartPrintf(USART_DEBUG, "OneNet_Publish\r\n");
        sprintf(
                buf,
                "{\"device\":%d,\"memberfid\":%d,\"temperature\":%.1f}",
                DEVICE_ID,usr.memberfid,usr.temperature);
        OneNet_Publish(*Topics, buf);  
        ESP8266_Clear();
        
        OLED_ShowChar(0,48,'(',16);
        OLED_ShowChinese(16,48,46,16);//数
        OLED_ShowChinese(32,48,47,16);//据
        OLED_ShowChinese(48,48,48,16);//已
        OLED_ShowChinese(64,48,49,16);//上
        OLED_ShowChinese(80,48,50,16);//传
        OLED_ShowChar(100,48,')',16);
        OLED_Refresh();
        
    }
    //延时参数最大1864
    delay_ms(1700);
    delay_ms(1700);
    delay_ms(1700);
}
