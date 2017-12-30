/**
  ******************************************************************************
  * @file    main.c
  * @author  DINGXU
  * @version V1.0
  * @date    2017-11-20调焦器
  * @brief   调焦器
  ******************************************************************************
  */ 
	
#include "stm32f10x.h"
#include "stm32f10x_it.h"
#include "stm32f10x_flash.h"
#include "bsp_led.h"
#include "bsp_SysTick.h"
#include "bsp_usart.h"
#include "bsp_hc05.h"
#include "bsp_usart_blt.h"
#include <stdio.h>
#include <stdlib.h>
#include "bsp_ds18b20.h"
#include <string.h>
#include "bsp_TiMbase.h"
#include "stdbool.h"
#define WriteFlashAddress    ((u32)0x0800FC00)//存储到最后一页，地址范围：0x0800 FC00~0x0800 FFFF

typedef union//union只能对第一个变量初始化
{
    struct
    {
      bool  Reverse;
			u8    Division;
			u16   Speed;
			int   Counter;
			char  DeviceName[10];   
    } CfgData;
		u32 SaveArray[5];
}UnionData; 

UnionData UD;// {0,0,32,32,0,"SSFocuser"};
extern u8 flagrun;          //串口接收标志
extern u8 blurunflag;       //蓝牙接收标志
bool bReverse = false;      //电机是否反向
bool bRisingEdge = false;   //是否为上升沿
bool bTempAvail=false;      //温度是否可用
bool bIsMoving = false;	    //运动状态标志true-stop,false-move
bool bIncreCount = true;    //是否递增计数true-递增计数 false-递减计数
//bool bMicroStep=false;    //细分是否可用Micro Step Mode
bool bAccFlag = false;
//bool bSavePos = false;

volatile u32 time = 0;      //计时变量 

char buffer[10];    				//设备名
char *pEquipment = "S2Focuser#";
char ReplyBuff[256] = {0};  //回复字符串
u8  uMoveState = 0;         //运动状态0-Idle,1-Start Move,2-Stop Move,3-Slew
u8  uSubdivision = 0;       //步进电机细分
u16 uSpeed  = 32 ;		    	//电机转动速度

int iStepCount = 0;         //当前步数
int iStepCmd = 0;           //命令步数
int iStepDelta = 0;         //步数差值
int iStepDeltaA = 0;         //步数差值
int iStepDeltaD = 0;         //步数差值

int iStartStep  = 0;         //加速开始步数    
int iStopStep  = 0;         //减速结束步数   

u8 Write_Flash(u32 *buff, u8 len);
u8 Write_Flash(u32 *buff, u8 len)
{    
	volatile FLASH_Status FLASHStatus;
	u32 Address = WriteFlashAddress;
	FLASHStatus = FLASH_COMPLETE;
	FLASH_Unlock();
	FLASH_ClearFlag(FLASH_FLAG_BSY | FLASH_FLAG_EOP | FLASH_FLAG_PGERR | FLASH_FLAG_WRPRTERR);
	FLASHStatus = FLASH_ErasePage(WriteFlashAddress);
	if(FLASHStatus == FLASH_COMPLETE)
	{
		u8 k;
		for(k=0;(k<len) && (FLASHStatus == FLASH_COMPLETE);k++)
		{
				FLASHStatus = FLASH_ProgramWord(Address, buff[k]);
				Address = Address + 4;
		}        
		FLASH_Lock();
	}
	else
	{
		return 0;
	}
	if(FLASHStatus == FLASH_COMPLETE)
	{
		return 1;
	}
	return 0;
}

void Read_Flash(u32 *buff, u8 len);
void Read_Flash(u32 *buff, u8 len)
{
	u32 Address = WriteFlashAddress;
	u8 k;
	for(k=0;k<len;k++)
	{
		buff[k] =  (*(vu32*) Address);
		Address += 4;     
	}
}

void ReadCfg()
{
	Read_Flash(UD.SaveArray, 5);
	bReverse = UD.CfgData.Reverse;
	uSubdivision = UD.CfgData.Division;
	uSpeed  = UD.CfgData.Speed;
	iStepCount = UD.CfgData.Counter;
	pEquipment = UD.CfgData.DeviceName;
}

void WriteCfg()
{
	Write_Flash(UD.SaveArray, 5);
}


void SetMode(int sMode);
void SetMode(int sMode)
{
	switch(sMode)
	{
		case 0:
		{
			/* M0=0 */
			GPIO_ResetBits(M0_GPIO_PORT, M0_GPIO_PIN);
			/* M1=0 */
			GPIO_ResetBits(M1_GPIO_PORT, M1_GPIO_PIN);
			/* M2=0 */
			GPIO_ResetBits(M2_GPIO_PORT, M2_GPIO_PIN);
		}
		break;
		
		case 2:
		{
			/* M0=1 */
			GPIO_SetBits(M0_GPIO_PORT, M0_GPIO_PIN);
			/* M1=0 */
			GPIO_ResetBits(M1_GPIO_PORT, M1_GPIO_PIN);
			/* M2=0 */
			GPIO_ResetBits(M2_GPIO_PORT, M2_GPIO_PIN);	
		}
		break;
		
		case 4:
		{
			/* M0=0 */
			GPIO_ResetBits(M0_GPIO_PORT, M0_GPIO_PIN);
			/* M1=1 */
			GPIO_SetBits(M1_GPIO_PORT, M1_GPIO_PIN);
			/* M2=0 */
			GPIO_ResetBits(M2_GPIO_PORT, M2_GPIO_PIN);	
		}
		break;
		
		case 8:
		{
			/* M0=1 */
			GPIO_SetBits(M0_GPIO_PORT, M0_GPIO_PIN);	   
			/* M1=1 */
			GPIO_SetBits(M1_GPIO_PORT, M1_GPIO_PIN);
			/* M2=0 */
			GPIO_ResetBits(M2_GPIO_PORT, M2_GPIO_PIN);	
		}
		break;
		
		case 16:
		{
			/* M0=0 */
			GPIO_ResetBits(M0_GPIO_PORT, M0_GPIO_PIN);	   
			/* M1=0 */
			GPIO_ResetBits(M1_GPIO_PORT, M1_GPIO_PIN);
			/* M2=1 */
			GPIO_SetBits(M2_GPIO_PORT, M2_GPIO_PIN);				
		}
		break;
		
		case 32:
		{
			/* M0=1 */
			GPIO_SetBits(M0_GPIO_PORT, M0_GPIO_PIN);   
			/* M1=1 */
			GPIO_SetBits(M1_GPIO_PORT, M1_GPIO_PIN);
			/* M2=1 */
			GPIO_SetBits(M2_GPIO_PORT, M2_GPIO_PIN);
		}
		break;
	}
}

//void SlewOut();
void SlewOut()//:F-#
{
	if(bReverse)
		GPIO_ResetBits(LED2_GPIO_PORT, LED2_GPIO_PIN);  //DRV8825dir信号
	else
		GPIO_SetBits(LED2_GPIO_PORT, LED2_GPIO_PIN);    //DRV8825dir信号
	ControlMotor(ENABLE);
	bIsMoving = true;
	bIncreCount = true;
}

//void SlewIn();
void SlewIn()//:F+#
{
	if(bReverse)
		GPIO_SetBits(LED2_GPIO_PORT, LED2_GPIO_PIN);    //DRV8825dir信号
	else
		GPIO_ResetBits(LED2_GPIO_PORT, LED2_GPIO_PIN);  //DRV8825dir信号
	ControlMotor(ENABLE);
	bIsMoving = true;
	bIncreCount = false;
}

void Pause()
{
	ControlMotor(DISABLE);
	bIsMoving = false;
	uMoveState = 0;
}

void Halt()
{
	ControlMotor(DISABLE);
	bIsMoving = false;
	uMoveState = 0;
	//到位后存储位置
	//ReadCfg();
	//UD.CfgData.Counter=iStepCount;
	//WriteCfg();
}
/*
bool Accelerate(u16 Speed);//加速算法
bool Accelerate(u16 Speed)//加速算法-返回true说明加速完成
{
	if(uSpeed<=Speed)//加速到Speed就停止加速
		return true;
	if(bIncreCount)
		iStepDeltaA = iStepCount - iStartStep;
	else
		iStepDeltaA = iStartStep - iStepCount;
	if(iStepDeltaA<10)
		uSpeed=uSpeed>64?64:uSpeed;
	else if(iStepDeltaA<20)
		uSpeed=uSpeed>32?32:uSpeed;
	else if(iStepDeltaA<30)
		uSpeed=uSpeed>16?16:uSpeed;
	else if(iStepDeltaA<40)
		uSpeed=uSpeed>8?8:uSpeed;
	else if(iStepDeltaA<50)
		uSpeed=uSpeed>4?4:uSpeed;
	else if(iStepDeltaA<60)
	  uSpeed=2;
	return false;
}

void Decelerate()//减速算法
{
	if(bIncreCount)
		iStepDeltaD = iStopStep - iStepCount;
	else
		iStepDeltaD = iStepCount - iStopStep;
	if(iStepDeltaD<=0)
		Halt();
	if(iStepDeltaD<10)
		uSpeed=uSpeed>64?uSpeed:64;
	else if(iStepDeltaD<20)
		uSpeed=uSpeed>32?uSpeed:32;
	else if(iStepDeltaD<30)
		uSpeed=uSpeed>16?uSpeed:16;
	else if(iStepDeltaD<40)
		uSpeed=uSpeed>8?uSpeed:8;
	else if(iStepDeltaD<50)
		uSpeed=uSpeed>4?uSpeed:4;
}
*/
void CmdProcess(unsigned char *RxBuffer);
void CmdProcess(unsigned char *RxBuffer)
{
	unsigned char ucTemp;
	ucTemp = *(RxBuffer+2);
	
	if (*(RxBuffer+0) == ':')
	{
		if (*(RxBuffer+1) == 'F')
		{
			switch (ucTemp)
			{
				case 'p':
					{
						if (iStepCount >= 0)
							sprintf(ReplyBuff,":p+%d#\n", iStepCount);
						else
							sprintf(ReplyBuff,":p%d#\n", iStepCount);
						break;
					}
				case 'B':
					{
						sprintf(ReplyBuff,":B%d#\n", bIsMoving);
						break;
					}			
				case 'P':       //:FPsxxxx#
					{
						if (RxBuffer[3] == '+')							
						{
							iStepCmd = atoi((char const *)RxBuffer+4);
						}
						if (RxBuffer[3] == '-')
						{
							iStepCmd = atoi((char const *)RxBuffer+4);
							iStepCmd = -iStepCmd;
						}
						iStepDelta = iStepCmd - iStepCount;
						iStartStep=iStepCount;
						iStopStep = iStepCmd;
						if(iStepDelta>0)
						{
							/*
							uMoveState = 1;
							if(iStepDelta>150)//大于150执行加减速
							{
								uSpeed=uSpeedM;
								bAccFlag=true;
							}
							else //小于100以64速度走
							{
								uSpeed=64;
								bAccFlag=false;
							}
							*/
							SlewOut();
						}
						else if(iStepDelta<0)
						{
							/*
							uMoveState = 1;
							if(iStepDelta<-150)//大于150执行加减速
							{
								uSpeed=uSpeedM;
								bAccFlag=true;
							}
							else //小于100以64速度走
							{
								uSpeed=64;
								bAccFlag=false;
							}
							*/
							SlewIn();
						}
						sprintf(ReplyBuff,":P#\n");
						break;
					}
				case 't':
					{
						if(bTempAvail)
							sprintf(ReplyBuff,":t%.2f#\n", DS18B20_Get_Temp());//保留2位小数
						else
							sprintf(ReplyBuff,":t%.2f#\n", 999.99);//温度传感器不可用返回999.99
						break;
					}
				case '-': 				//:F-#  
					{
						//uMoveState = 2;
						iStartStep=iStepCount;
						SlewOut();
						sprintf(ReplyBuff,":-#\n");
						break;
					}
				case '+': 				//:F+#    
					{
						//uMoveState = 2;
						iStartStep=iStepCount;
						SlewIn();
						sprintf(ReplyBuff,":+#\n");
						break;
					}
				case 'Q': 				//:FQ#    
					{
						//uMoveState = 3;
						if(bIncreCount)
							iStopStep = iStepCount+50;//30步之内停下来
						else
							iStopStep = iStepCount-50;//30步之内停下来
						Halt();
						sprintf(ReplyBuff,":Q#\n");
						break;
					}
				case '?':
					{
						sprintf(ReplyBuff, ":?%s#\n", pEquipment);
						break;
					}
				case 'D':   				//:FD�*�*�*# 
					{
						strcpy(buffer, (char *)RxBuffer+3);
						pEquipment = buffer;	
						//ReadCfg();
						//strcpy(UD.CfgData.DeviceName, (char *)RxBuffer+3);
						//pEquipment = UD.CfgData.DeviceName;	
						//WriteCfg();
						sprintf(ReplyBuff,":D#\n");
						break;
					}
				case 'R': 				//:FR#    
					{
						Pause();
						bReverse = !bReverse; 
						//ReadCfg();
						//UD.CfgData.Reverse=bReverse;
						//WriteCfg();
						sprintf(ReplyBuff,":R#\n");
						break;
					}	
				case 'S':       //:FS+01000#;
					{
						if (RxBuffer[3] == '+')//只允许定义正值
						{
							Pause();
							iStepCount = atoi((char const *)RxBuffer+4);
							//ReadCfg();
							//UD.CfgData.Counter=iStepCount;
							//WriteCfg();
							sprintf(ReplyBuff,":S#\n");
						}
						break;							
					}
				case 'M':     //:FM2#
					{
						Pause();
						uSubdivision = atoi((char const *)RxBuffer+3);
						SetMode(uSubdivision);
						//ReadCfg();
						//UD.CfgData.Division=uSubdivision;
						//WriteCfg();
						sprintf(ReplyBuff,":M#\n");
						break;
					}
				case 'V': //:FV256#
					{
						Pause();
						uSpeed = atoi((char const *)RxBuffer+3);
						if(uSpeed<1) 
							uSpeed = 1;
						else if(uSpeed>10000)
							uSpeed = 10000;
						printf("%d",uSpeed);
						SetSpeedMoter(uSpeed);
						//ReadCfg();
						//UD.CfgData.SpeedM=uSpeedM;
						//WriteCfg();
						sprintf(ReplyBuff, ":V#\n");
						break;
					}
				default:
					{
						sprintf(ReplyBuff, ":U#\n");
						break;
					}
			}
		}
	}
	
	RxBuffer = NULL;
}

int main()
{	
	LED_GPIO_Config();

	SysTick_Init();
	
	USART_Config();

	HC05_Init();

	BASIC_TIM_Init();
	
	if( DS18B20_Init())	
		//printf("\r\n no ds18b20 exit \r\n");
		bTempAvail=false;
	else
		bTempAvail=true;
	//printf("\r\n ds18b20 exit \r\n");
	SetMode(uSubdivision);//初始化细分
	
	//ReadCfg();
	//ControlMotor(ENABLE);
	
	while(1)
	{	
		//处理串口1数据
		if (flagrun == 1)
		{
			CmdProcess(UART_RxBuffer);
			printf("%s", ReplyBuff);
			flagrun = 0;
			uart_FlushRxBuffer();
			ReplyBuff[0] = '\0';
			
		}	
		/*	
	  //处理串口2数据	
	  if (blurunflag == 1)
	  {     
			CmdProcess(uart_buff);
			HC05_SendString(ReplyBuff);
			blurunflag = 0;
			clean_rebuff();
			ReplyBuff[0] = '\0';
	  }
	  
     //电机控制
    if ( time > uSpeed ) 
    {
			time = 0;
			if (bRisingEdge)//高电平变低电平计数（下降沿）
			{
				digitalLo(LED3_GPIO_PORT,LED3_GPIO_PIN);//输出低电平
				bRisingEdge = false;
												
				if ((bIsMoving == true) && (bIncreCount == true))
				{
						iStepCount++;
				}	
				else if ((bIsMoving == true) && (bIncreCount == false) )
				{
						iStepCount--;	
				}
			}			
			else
			{
				digitalHi(LED3_GPIO_PORT,LED3_GPIO_PIN);//输出高电平
				bRisingEdge = true;
			}
			
			//LED3_TOGGLE; //step	
			
			if(uMoveState==1)
			{
				//bAccFlag=Accelerate(uSpeedM);
				if(bAccFlag)//bAccFlag==true-加速完成
					Accelerate(uSpeedM);
				Decelerate();
			}	
			else if(uMoveState==2)
			{
				Accelerate(uSpeedS);
			}
			else if(uMoveState==3)
			{
				Decelerate();
			}
    } 
*/		
	}
}



/*********************************************END OF FILE**********************/
