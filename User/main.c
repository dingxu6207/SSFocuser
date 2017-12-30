/**
  ******************************************************************************
  * @file    main.c
  * @author  DINGXU
  * @version V1.0
  * @date    2017-11-20µ÷½¹Æ÷
  * @brief   µ÷½¹Æ÷
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
#define WriteFlashAddress    ((u32)0x0800FC00)//´æ´¢µ½×îºóÒ»Ò³£¬µØÖ··¶Î§£º0x0800 FC00~0x0800 FFFF

typedef union//unionÖ»ÄÜ¶ÔµÚÒ»¸ö±äÁ¿³õÊ¼»¯
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
extern u8 flagrun;          //´®¿Ú½ÓÊÕ±êÖ¾
extern u8 blurunflag;       //À¶ÑÀ½ÓÊÕ±êÖ¾
bool bReverse = false;      //µç»úÊÇ·ñ·´Ïò
bool bRisingEdge = false;   //ÊÇ·ñÎªÉÏÉýÑØ
bool bTempAvail=false;      //ÎÂ¶ÈÊÇ·ñ¿ÉÓÃ
bool bIsMoving = false;	    //ÔË¶¯×´Ì¬±êÖ¾true-stop,false-move
bool bIncreCount = true;    //ÊÇ·ñµÝÔö¼ÆÊýtrue-µÝÔö¼ÆÊý false-µÝ¼õ¼ÆÊý
//bool bMicroStep=false;    //Ï¸·ÖÊÇ·ñ¿ÉÓÃMicro Step Mode
bool bAccFlag = false;
//bool bSavePos = false;

volatile u32 time = 0;      //¼ÆÊ±±äÁ¿ 

char buffer[10];    				//Éè±¸Ãû
char *pEquipment = "S2Focuser#";
char ReplyBuff[256] = {0};  //»Ø¸´×Ö·û´®
u8  uMoveState = 0;         //ÔË¶¯×´Ì¬0-Idle,1-Start Move,2-Stop Move,3-Slew
u8  uSubdivision = 0;       //²½½øµç»úÏ¸·Ö
u16 uSpeed  = 32 ;		    	//µç»ú×ª¶¯ËÙ¶È

int iStepCount = 0;         //µ±Ç°²½Êý
int iStepCmd = 0;           //ÃüÁî²½Êý
int iStepDelta = 0;         //²½Êý²îÖµ
int iStepDeltaA = 0;         //²½Êý²îÖµ
int iStepDeltaD = 0;         //²½Êý²îÖµ

int iStartStep  = 0;         //¼ÓËÙ¿ªÊ¼²½Êý    
int iStopStep  = 0;         //¼õËÙ½áÊø²½Êý   

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
		GPIO_ResetBits(LED2_GPIO_PORT, LED2_GPIO_PIN);  //DRV8825dirÐÅºÅ
	else
		GPIO_SetBits(LED2_GPIO_PORT, LED2_GPIO_PIN);    //DRV8825dirÐÅºÅ
	ControlMotor(ENABLE);
	bIsMoving = true;
	bIncreCount = true;
}

//void SlewIn();
void SlewIn()//:F+#
{
	if(bReverse)
		GPIO_SetBits(LED2_GPIO_PORT, LED2_GPIO_PIN);    //DRV8825dirÐÅºÅ
	else
		GPIO_ResetBits(LED2_GPIO_PORT, LED2_GPIO_PIN);  //DRV8825dirÐÅºÅ
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
	//µ½Î»ºó´æ´¢Î»ÖÃ
	//ReadCfg();
	//UD.CfgData.Counter=iStepCount;
	//WriteCfg();
}
/*
bool Accelerate(u16 Speed);//¼ÓËÙËã·¨
bool Accelerate(u16 Speed)//¼ÓËÙËã·¨-·µ»ØtrueËµÃ÷¼ÓËÙÍê³É
{
	if(uSpeed<=Speed)//¼ÓËÙµ½Speed¾ÍÍ£Ö¹¼ÓËÙ
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

void Decelerate()//¼õËÙËã·¨
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
							if(iStepDelta>150)//´óÓÚ150Ö´ÐÐ¼Ó¼õËÙ
							{
								uSpeed=uSpeedM;
								bAccFlag=true;
							}
							else //Ð¡ÓÚ100ÒÔ64ËÙ¶È×ß
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
							if(iStepDelta<-150)//´óÓÚ150Ö´ÐÐ¼Ó¼õËÙ
							{
								uSpeed=uSpeedM;
								bAccFlag=true;
							}
							else //Ð¡ÓÚ100ÒÔ64ËÙ¶È×ß
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
							sprintf(ReplyBuff,":t%.2f#\n", DS18B20_Get_Temp());//±£Áô2Î»Ð¡Êý
						else
							sprintf(ReplyBuff,":t%.2f#\n", 999.99);//ÎÂ¶È´«¸ÐÆ÷²»¿ÉÓÃ·µ»Ø999.99
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
							iStopStep = iStepCount+50;//30²½Ö®ÄÚÍ£ÏÂÀ´
						else
							iStopStep = iStepCount-50;//30²½Ö®ÄÚÍ£ÏÂÀ´
						Halt();
						sprintf(ReplyBuff,":Q#\n");
						break;
					}
				case '?':
					{
						sprintf(ReplyBuff, ":?%s#\n", pEquipment);
						break;
					}
				case 'D':   				//:FDå*å*å*# 
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
						if (RxBuffer[3] == '+')//Ö»ÔÊÐí¶¨ÒåÕýÖµ
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
	SetMode(uSubdivision);//³õÊ¼»¯Ï¸·Ö
	
	//ReadCfg();
	//ControlMotor(ENABLE);
	
	while(1)
	{	
		//´¦Àí´®¿Ú1Êý¾Ý
		if (flagrun == 1)
		{
			CmdProcess(UART_RxBuffer);
			printf("%s", ReplyBuff);
			flagrun = 0;
			uart_FlushRxBuffer();
			ReplyBuff[0] = '\0';
			
		}	
		/*	
	  //´¦Àí´®¿Ú2Êý¾Ý	
	  if (blurunflag == 1)
	  {     
			CmdProcess(uart_buff);
			HC05_SendString(ReplyBuff);
			blurunflag = 0;
			clean_rebuff();
			ReplyBuff[0] = '\0';
	  }
	  
     //µç»ú¿ØÖÆ
    if ( time > uSpeed ) 
    {
			time = 0;
			if (bRisingEdge)//¸ßµçÆ½±äµÍµçÆ½¼ÆÊý£¨ÏÂ½µÑØ£©
			{
				digitalLo(LED3_GPIO_PORT,LED3_GPIO_PIN);//Êä³öµÍµçÆ½
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
				digitalHi(LED3_GPIO_PORT,LED3_GPIO_PIN);//Êä³ö¸ßµçÆ½
				bRisingEdge = true;
			}
			
			//LED3_TOGGLE; //step	
			
			if(uMoveState==1)
			{
				//bAccFlag=Accelerate(uSpeedM);
				if(bAccFlag)//bAccFlag==true-¼ÓËÙÍê³É
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
