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
#include "Exti44E.h"

#define WriteFlashAddress    ((u32)0x0800FC00)//´æ´¢µ½×îºóÒ»Ò³£¬µØÖ··¶Î§£º0x0800 FC00~0x0800 FFFF

typedef union//unionÖ»ÄÜ¶ÔµÚÒ»¸ö±äÁ¿³õÊ¼»¯
{
    struct
    {
      u8    Reverse;
			u8    Division;
			u16   Speed;
			int   Counter;
			char  DeviceName[12];   
    } CfgData;
		u32 SaveArray[5];
}UnionData; 

UnionData UD;
extern u8 flagrun;          //´®¿Ú½ÓÊÕ±êÖ¾
extern u8 blurunflag;       //À¶ÑÀ½ÓÊÕ±êÖ¾
bool bReverse = false;      //µç»úÊÇ·ñ·´Ïò
bool bRisingEdge = false;   //ÊÇ·ñÎªÉÏÉýÑØ
bool bTempAvail=false;      //ÎÂ¶ÈÊÇ·ñ¿ÉÓÃ
bool bIsMoving = false;	    //ÔË¶¯×´Ì¬±êÖ¾true-stop,false-move
bool bIncreCount = true;    //ÊÇ·ñµÝÔö¼ÆÊýtrue-µÝÔö¼ÆÊý false-µÝ¼õ¼ÆÊý

char *pEquipment = "SS Focuser";
char ReplyBuff[256] = {0};  //»Ø¸´×Ö·û´®
u8  uMoveState = 0;         //ÔË¶¯×´Ì¬0-Idle,1-Start Move,2-Stop Move,3-Slew
u8  uSubdivision = 0;       //²½½øµç»úÏ¸·Ö
u16 uSpeed  = 25 ;		    	//µç»ú×ª¶¯ËÙ¶È
u16 uSpeedTmp  = 1 ;

int iStepCount = 0;         //µ±Ç°²½Êý
int iStepCmd = 0;           //ÃüÁî²½Êý
int iStepDelta = 0;         //²½Êý²îÖµ
int iStartStep  = 0;         //¼ÓËÙ¿ªÊ¼²½Êý    
int iStopStep  = 0;         //¼õËÙ½áÊø²½Êý   
u8  i=0;

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

void ReadCfg(void)
{
	Read_Flash(UD.SaveArray, 5);
}

void WriteCfg(void)
{
	Write_Flash(UD.SaveArray, 5);
}

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

void SetSpeed(u16 SetSpeed)
{
	SetSpeedMoter(10000/SetSpeed);
}

void SlewOut(void)//:F-#
{
	if(bReverse)
		GPIO_ResetBits(LED2_GPIO_PORT, LED2_GPIO_PIN);  //DRV8825dirÐÅºÅ
	else
		GPIO_SetBits(LED2_GPIO_PORT, LED2_GPIO_PIN);    //DRV8825dirÐÅºÅ
	ControlMotor(ENABLE);
	bIsMoving = true;
	bIncreCount = true;
}

void SlewIn(void)//:F+#
{
	if(bReverse)
		GPIO_SetBits(LED2_GPIO_PORT, LED2_GPIO_PIN);    //DRV8825dirÐÅºÅ
	else
		GPIO_ResetBits(LED2_GPIO_PORT, LED2_GPIO_PIN);  //DRV8825dirÐÅºÅ
	ControlMotor(ENABLE);
	bIsMoving = true;
	bIncreCount = false;
}

void Pause(void)
{
	ControlMotor(DISABLE);
	bIsMoving = false;
	uMoveState = 0;
}

void Halt(void)
{
	ControlMotor(DISABLE);
	UD.CfgData.Counter=iStepCount;
	WriteCfg();
	bIsMoving = false;
	uMoveState = 0;
}

void Accelerate(int iDelta)//¼ÓËÙËã·¨
{
	if(uSpeedTmp>=uSpeed)
		return;
	for(i=1;i<=80;i++)
	{
		if(iDelta<i*5)
		{
			uSpeedTmp=i*5;
			SetSpeed(uSpeedTmp);
		}
	}
}

void Decelerate(int iDelta)//¼õËÙËã·¨
{
	if(iDelta<=0)
		Halt();
	for(i=1;i<=80;i++)
	{
		if(iDelta<i*5)
		{
			uSpeedTmp=i*5;
			if(uSpeedTmp>=uSpeed)
				uSpeedTmp=uSpeed;
			SetSpeed(uSpeedTmp);
		}
	}
}

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
						iStartStep=iStepCount;
						iStopStep = iStepCmd;
						uSpeedTmp=1;
						if(iStepCmd > iStepCount)
						{
							uMoveState = 1;
							SlewOut();
						}
						else if(iStepCmd < iStepCount)
						{
							uMoveState = 2;
							SlewIn();
						}
						sprintf(ReplyBuff,":P#\n");
						break;
					}
				case '-': 				//:F-#  
					{
						uSpeedTmp=1;
						uMoveState = 3;
						iStartStep=iStepCount;
						SlewOut();
						sprintf(ReplyBuff,":-#\n");
						break;
					}
				case '+': 				//:F+#    
					{
						uSpeedTmp=1;
						uMoveState = 4;
						iStartStep=iStepCount;
						SlewIn();
						sprintf(ReplyBuff,":+#\n");
						break;
					}
				case 'Q': 				//:FQ#    
					{
						uSpeedTmp=1;
						switch(uMoveState)
						{
							case 1:
							{
								iStopStep = iStepCount+100;
								uMoveState = 5;
								break;
							}
							case 2:
							{
								iStopStep = iStepCount-100;
								uMoveState = 5;
								break;
							}
							case 3:
							{
								iStopStep = iStepCount+100;
								uMoveState = 5;
								break;
							}
							case 4:
							{
								iStopStep = iStepCount-100;
								uMoveState = 6;
								break;
							}
							default:
							{
								uMoveState = 0;
								break;
							}
						}
						sprintf(ReplyBuff,":Q#\n");
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
				case '?':
					{
						sprintf(ReplyBuff, ":?%s#\n", pEquipment);
						break;
					}
				case 'D':   				//:FDå*å*å*# 
					{
						strcpy(UD.CfgData.DeviceName, (char *)RxBuffer+3);
						pEquipment = UD.CfgData.DeviceName;	
						WriteCfg();
						sprintf(ReplyBuff,":D#\n");
						break;
					}
				case 'R': 				//:FR#    
					{
						bReverse = !bReverse; 
						UD.CfgData.Reverse=bReverse?1:0;
						WriteCfg();
						sprintf(ReplyBuff,":R#\n");
						break;
					}	
				case 'S':       //:FS+01000#;
					{
						if (RxBuffer[3] == '+')//Ö»ÔÊÐí¶¨ÒåÕýÖµ
						{
							Halt();
							iStepCount = atoi((char const *)RxBuffer+4);
							UD.CfgData.Counter=iStepCount;
							WriteCfg();
							sprintf(ReplyBuff,":S#\n");
						}
						break;							
					}
				case 'M':     //:FM2#
					{
						uSubdivision = atoi((char const *)RxBuffer+3);
						SetMode(uSubdivision);
						UD.CfgData.Division=uSubdivision;
						WriteCfg();
						sprintf(ReplyBuff,":M#\n");
						break;
					}
				case 'V': //:FV256#
					{
						uSpeed = atoi((char const *)RxBuffer+3);
						if(uSpeed<1) 
							uSpeed = 1;
						else if(uSpeed>400)
							uSpeed = 400;
						SetSpeed(uSpeed);
						UD.CfgData.Speed=uSpeed;
						WriteCfg();
						sprintf(ReplyBuff, ":V#\n");
						break;
					}
				case 'G': //:FV256#
					{
						ReadCfg(); 
						sprintf(ReplyBuff, ":GReverse->%d Subdivision->%d Speed->%d StepCount->%d DeviceName->%s#\n",UD.CfgData.Reverse,UD.CfgData.Division,UD.CfgData.Speed,UD.CfgData.Counter,UD.CfgData.DeviceName);
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
	
	EXTI_44E_Config();
	
	if( DS18B20_Init())	
		//printf("\r\n no ds18b20 exit \r\n");
		bTempAvail=false;
	else
		bTempAvail=true;
	//printf("\r\n ds18b20 exit \r\n");
	
	ReadCfg();
	if(UD.CfgData.Reverse==255)//Î´³õÊ¼»¯¹ý¾Í³õÊ¼»¯
	{
		UD.CfgData.Reverse=(bReverse==true)?1:0;
		UD.CfgData.Division=uSubdivision;
		UD.CfgData.Speed=uSpeed;
		UD.CfgData.Counter=iStepCount;
		strcpy(UD.CfgData.DeviceName, pEquipment);
		WriteCfg();
	}
	
	bReverse = UD.CfgData.Reverse==0?false:true;
	uSubdivision = UD.CfgData.Division;
	uSpeed  = UD.CfgData.Speed;
	iStepCount = UD.CfgData.Counter;
	pEquipment = UD.CfgData.DeviceName;
	
	SetMode(uSubdivision);
	SetSpeed(uSpeed);
	
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
	
	  //´¦Àí´®¿Ú2Êý¾Ý	
	  if (blurunflag == 1)
	  {     
			CmdProcess(uart_buff);
			HC05_SendString(ReplyBuff);
			blurunflag = 0;
			clean_rebuff();
			ReplyBuff[0] = '\0';
	  }
	  switch(uMoveState)
		{
			case 1://Move Out
			{
				iStepDelta = iStepCmd - iStepCount;
				if(iStepDelta<=200)
					Decelerate(iStepDelta);//¼õËÙÓÅÏÈ
				else
					Accelerate(iStepDelta);
				break;
			}	
			case 2://Move In
			{
				iStepDelta = iStepCount - iStepCmd;
				if(iStepDelta<=200)
					Decelerate(iStepDelta);//¼õËÙÓÅÏÈ
				else
					Accelerate(iStepDelta);
				break;
			}
			case 3://Slew Out
			{
				iStepDelta = iStepCount - iStartStep;
				Accelerate(iStepDelta);
				break;
			}
			case 4://Slew In
			{
				iStepDelta = iStartStep - iStepCount;
				Accelerate(iStepDelta);
				break;
			}
			case 5://
			{
				iStepDelta = iStopStep - iStepCount;
				Decelerate(iStepDelta);//¼õËÙÓÅÏÈ
				break;
			}
			case 6://
			{
				iStepDelta = iStepCount - iStopStep;
				Decelerate(iStepDelta);//¼õËÙÓÅÏÈ
				break;
			}
			default:
			{
				uMoveState=0;
				break;
			}
		}
	}
}



/*********************************************END OF FILE**********************/
