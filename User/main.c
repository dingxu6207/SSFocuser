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
#include "Exti44E.h"
#include "WifiUsart.h"
#include "ESP8266.h"

#define WriteFlashAddress    ((u32)0x0800FC00)//存储到最后一页，地址范围：0x0800 FC00~0x0800 FFFF


typedef union//union只能对第一个变量初始化
{
    struct
    {
			//设备信息
			char  Device[16];
			char  Version[16];	
			//WiFi信息
			char  WFSID[16];   
			char  WFPWD[16];
			char  WFRIP[16];
			char  WFRPO[8];
			//蓝牙信息
			char  BTSID[16];   
			char  BTPWD[16];
			//电机设置
      u8    IsReverse;
			u8    SubDivision;
			u16   MotorSpeed;
			//运动状态
			int   Position;
    } CfgData;
		u32 SaveArray[32];
}UnionData; 

UnionData UD;

unsigned char UART_RxPtr_Prv=0;//上次读取位置
unsigned char BLTUART_RxPtr_Prv=0;//上次读取位置
unsigned char WIFIUART_RxPtr_Prv=0;//上次读取位置
unsigned char UART_RxCmd=0;//未读命令个数
unsigned char BLTUART_RxCmd=0;//未读命令个数
unsigned char WIFIUART_RxCmd=0;//未读命令个数

bool bIsReverse = false;    //电机是否反向
bool bIsMoving = false;	    //运动状态标志true-stop,false-move
bool bIncreCount = true;    //是否递增计数true-递增计数 false-递减计数
bool bTempAvail=false;      //温度是否可用

char *pEquipment = "SSFocuser";
char *pVersion = "BT 1.0";//SE-串口版,BT-蓝牙版,WF-WiFi版,不能在线修改
char *pWFSID = "SSRouter";  //WiFi SSID
char *pWFPWD = "graycode";  //WiFi PWD
char *pWFRIP = "192.168.1.100"; //Remote IP
char *pWFRPO = "8088";          //Remote Port
char *pBTSID = "SSFocuser"; //BT SSID
char *pBTPWD = "1234";      //BT PWD
char ReplyBuff[128] = {0};  //回复字符串
char CmdBuff[32] = {0};     //回复字符串

u8  uMoveState = 0;         //运动状态0-Idle,1-Start Move,2-Stop Move,3-Slew
u8  uSubdivision = 1;       //步进电机细分
u8  uSubdivisionTmp = 1;
u16 uSpeed  = 25 ;		    	//电机转动速度
u16 uSpeedCur  = 1 ;
u16 uSpeedTmp  = 1 ;
//unsigned char i=0,j=0,k=0;

int iStepCount = 0;         //当前步数
int iStepCmd = 0;           //命令步数
int iStepDelta = 0;         //步数差值
int iHalfStepDelta = 0;    
int iStartStep  = 0;         //加速开始步数    
int iStopStep  = 0;         //减速结束步数 

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
	Read_Flash(UD.SaveArray, 32);
}

void WriteCfg(void)
{
	Write_Flash(UD.SaveArray, 32);
}
void SetBT(char *SID,char *PWD)
{
	;
}
void SetWiFi(char *SID,char *PWD)
{
	;
}
void SetMode(int sMode)
{
	switch(sMode)
	{
		case 1:
		{
			/* M0=0 */
			GPIO_ResetBits(M0_GPIO_PORT, M0_GPIO_PIN);
			/* M1=0 */
			GPIO_ResetBits(M1_GPIO_PORT, M1_GPIO_PIN);
			/* M2=0 */
			GPIO_ResetBits(M2_GPIO_PORT, M2_GPIO_PIN);
			break;
		}
		case 2:
		{
			/* M0=1 */
			GPIO_SetBits(M0_GPIO_PORT, M0_GPIO_PIN);
			/* M1=0 */
			GPIO_ResetBits(M1_GPIO_PORT, M1_GPIO_PIN);
			/* M2=0 */
			GPIO_ResetBits(M2_GPIO_PORT, M2_GPIO_PIN);	
			break;
		}
		case 4:
		{
			/* M0=0 */
			GPIO_ResetBits(M0_GPIO_PORT, M0_GPIO_PIN);
			/* M1=1 */
			GPIO_SetBits(M1_GPIO_PORT, M1_GPIO_PIN);
			/* M2=0 */
			GPIO_ResetBits(M2_GPIO_PORT, M2_GPIO_PIN);	
			break;
		}
		case 8:
		{
			/* M0=1 */
			GPIO_SetBits(M0_GPIO_PORT, M0_GPIO_PIN);	   
			/* M1=1 */
			GPIO_SetBits(M1_GPIO_PORT, M1_GPIO_PIN);
			/* M2=0 */
			GPIO_ResetBits(M2_GPIO_PORT, M2_GPIO_PIN);	
			break;
		}
		case 16:
		{
			/* M0=0 */
			GPIO_ResetBits(M0_GPIO_PORT, M0_GPIO_PIN);	   
			/* M1=0 */
			GPIO_ResetBits(M1_GPIO_PORT, M1_GPIO_PIN);
			/* M2=1 */
			GPIO_SetBits(M2_GPIO_PORT, M2_GPIO_PIN);		
			break;
		}
		case 32:
		{
			/* M0=1 */
			GPIO_SetBits(M0_GPIO_PORT, M0_GPIO_PIN);   
			/* M1=1 */
			GPIO_SetBits(M1_GPIO_PORT, M1_GPIO_PIN);
			/* M2=1 */
			GPIO_SetBits(M2_GPIO_PORT, M2_GPIO_PIN);
			break;
		}
	}
}

void SetSpeed(u16 SetSpeed)
{
	SetSpeedMoter(10000/SetSpeed);
}

void SlewOut(void)//:F-#
{
	bIsMoving = true;
	bIncreCount = true;
	GPIO_ResetBits(LED5_GPIO_PORT, LED5_GPIO_PIN); //DRV8825使能信号
	if(bIsReverse)
		GPIO_ResetBits(LED2_GPIO_PORT, LED2_GPIO_PIN);  //DRV8825dir信号
	else
		GPIO_SetBits(LED2_GPIO_PORT, LED2_GPIO_PIN);    //DRV8825dir信号
	SysTick_Delay_Ms(20);
	ControlMotor(ENABLE);
}

void SlewIn(void)//:F+#
{
	bIsMoving = true;
	bIncreCount = false;
	GPIO_ResetBits(LED5_GPIO_PORT, LED5_GPIO_PIN); //DRV8825使能信号
	if(bIsReverse)
		GPIO_SetBits(LED2_GPIO_PORT, LED2_GPIO_PIN);    //DRV8825dir信号
	else
		GPIO_ResetBits(LED2_GPIO_PORT, LED2_GPIO_PIN);  //DRV8825dir信号
	SysTick_Delay_Ms(20);
	ControlMotor(ENABLE);
}

void Halt(void)
{
	ControlMotor(DISABLE);
	bIsMoving = false;
	uMoveState = 0;
	//UD.CfgData.Position=iStepCount;
	//WriteCfg();	
}

void Accelerate(int iDelta)//加速算法
{
	uSpeedTmp=(iDelta/5)*5+5;
	if(uSpeedTmp<uSpeed)
	{
		uSpeedCur=uSpeedTmp;
		SetSpeed(uSpeedTmp);
	}
}

void Decelerate(int iDelta)//减速算法
{
	if(iDelta<=0)
		Halt();
	uSpeedTmp=(iDelta/5)*5+10;
	//if(uSpeedTmp<uSpeed)
	if(uSpeedTmp<uSpeedCur)
		SetSpeed(uSpeedTmp);
}

u8 Next(u8 Prv)
{
	if(Prv<255)
		return Prv+1;
	else //255
		return 0;
}

u8 ReadCmd(unsigned char *RxBuffer,unsigned char *Ptr)
{
	u8 Flag=0,i=0;
	for(;;)
	{
		//下面三条判断的顺序不能调整
		if(RxBuffer[*Ptr]==35)//#结束命令
		{ 
			if(Flag==1)//命令完整
				Flag=2;
			else
				Flag=3;
		}
		if(Flag==1)//:F后才开始读取命令字符串
		{
			CmdBuff[i]=RxBuffer[*Ptr];
			i++;
		}
		if((RxBuffer[*Ptr]==58)&&(RxBuffer[Next(*Ptr)]==70))//:F
		{
			*Ptr=Next(*Ptr);
			Flag=1;i=0;
		}
		//循环读取串口缓冲区
		*Ptr=Next(*Ptr);
		if(Flag>1)//跳出循环
			break;
	}
	return i;
}

bool CmdProcess(unsigned char *RxBuffer,unsigned char *Ptr)
{
	if(ReadCmd(RxBuffer,Ptr)<1)
	{
		memset(CmdBuff,0,32);
		return false;
	}
	switch (CmdBuff[0])
	{
		case 'P':    //Move command
			{
				iStepCmd = atoi((char const *)CmdBuff+2);
				if (CmdBuff[1] == '-')
					iStepCmd = -iStepCmd;
				iStartStep=iStepCount;
				iStopStep = iStepCmd;
				SetSpeed(1);
				uSpeedCur=1;
				uSpeedTmp=1;
				iHalfStepDelta=(iStopStep - iStartStep)/2;
				if(iStopStep > iStartStep)
				{
					uMoveState = 1;
					SlewOut();
				}
				else if(iStopStep < iStartStep)
				{
					uMoveState = 2;
					iHalfStepDelta = -iHalfStepDelta;
					SlewIn();
				}
				sprintf(ReplyBuff,":P#\n");
				break;
			}
		case 'p':  //Get Position and Moving state
			{
				if (iStepCount >= 0)
					sprintf(ReplyBuff,":p+%d-%d#\n", iStepCount, bIsMoving);
				else
					sprintf(ReplyBuff,":p%d-%d#\n", iStepCount, bIsMoving);
				break;
			}
		case 'S':    //Define Position
			{
				Halt();
				iStepCount = atoi((char const *)CmdBuff+2);
				if (CmdBuff[1] == '-')
					iStepCount=-iStepCount;
				//UD.CfgData.Position=iStepCount;
				//WriteCfg();
				sprintf(ReplyBuff,":S#\n");
				break;							
			}
		case '+': 	//Slew Out		 
			{
				if(uMoveState != 3)
				{
					uMoveState = 3;
					iStartStep=iStepCount;
					SetSpeed(1);
					uSpeedCur=1;
					uSpeedTmp=1;
					SlewOut();
					sprintf(ReplyBuff,":+#\n");
				}
				break;
			}
		case '-':   //Slew In
			{
				if(uMoveState != 4)
				{
					uMoveState = 4;
					iStartStep=iStepCount;
					SetSpeed(1);
					uSpeedCur=1;
					uSpeedTmp=1;
					SlewIn();
					sprintf(ReplyBuff,":-#\n");
				}
				break;
			}
		case 'Q': 	//Halt    
			{
				switch(uMoveState)
				{
					case 1:
					{
						//iStopStep = iStepCount+uSpeed;
						iStopStep = iStepCount+uSpeedCur;
						uMoveState = 5;
						break;
					}
					case 2:
					{
						//iStopStep = iStepCount-uSpeed;
						iStopStep = iStepCount-uSpeedCur;
						uMoveState = 6;
						break;
					}
					case 3:
					{
						//iStopStep = iStepCount+uSpeed;
						iStopStep = iStepCount+uSpeedCur;
						uMoveState = 5;
						break;
					}
					case 4:
					{
						//iStopStep = iStepCount-uSpeed;
						iStopStep = iStepCount-uSpeedCur;
						uMoveState = 6;
						break;
					}
					default:
					{
						Halt();
						//uMoveState = 0;
						break;
					}
				}
				sprintf(ReplyBuff,":Q#\n");
				break;
			}
		case 't':   //Get temperature
			{
				if(bTempAvail)
					sprintf(ReplyBuff,":t%.2f#\n", DS18B20_Get_Temp());//保留2位小数
				else
					sprintf(ReplyBuff,":t%.2f#\n", 999.99);//温度传感器不可用返回999.99
				break;
			}
		case '?':  //ACK , return version
			{
				sprintf(ReplyBuff, ":?%s#\n", pVersion);
				break;
			}
		case 'D':  //Define device name 			
			{
				strcpy(UD.CfgData.Device, (char *)CmdBuff+1);
				pEquipment = UD.CfgData.Device;	
				WriteCfg();
				sprintf(ReplyBuff,":D#\n");
				break;
			}
		case 'R': //Motor Reverse			
			{
				bIsReverse = !bIsReverse; 
				UD.CfgData.IsReverse=bIsReverse?1:0;
				WriteCfg();
				sprintf(ReplyBuff,":R#\n");
				break;
			}
		case 'r': //Get Reverse	Status		
			{
				sprintf(ReplyBuff,":r%d#\n",bIsReverse);
				break;
			}
		case 'M': //Motor Subdivision
			{
				Halt();
				uSubdivisionTmp = atoi((char const *)CmdBuff+1);
				switch(uSubdivisionTmp)
				{
					case 1:
					case 2:
					case 4:
					case 8:
					case 16:
					case 32:
					{
						//计算当前细分下的计数值
						iStepCount = (iStepCount *uSubdivisionTmp)/uSubdivision;
						uSubdivision=uSubdivisionTmp;//存储细分值
						SetMode(uSubdivision);
						UD.CfgData.SubDivision=uSubdivision;
						WriteCfg();
						sprintf(ReplyBuff,":M#\n");
						break;
					}
				}
				break;
			}
		case 'V': //Motor Velocity
			{
				uSpeed = atoi((char const *)CmdBuff+1);
				if(uSpeed<1) 
					uSpeed = 1;
				else if(uSpeed>400)
					uSpeed = 400;
				//SetSpeed(uSpeed);
				UD.CfgData.MotorSpeed=uSpeed;
				WriteCfg();
				sprintf(ReplyBuff, ":V#\n");
				break;
			}
		case 'W': //WiFi settiog  			
			{
				if (CmdBuff[1] == '0')
				{
					SetWifiName((unsigned char *)pWFSID);
					SetWifiCode((unsigned char *)pWFPWD);
					SetRemoteHost((unsigned char *)pWFRIP,(unsigned char *)pWFRPO);
					SetNameCode();
					SetWifiConnect();	
				}
				else if (CmdBuff[1] == '1')							
				{
					strcpy(UD.CfgData.WFSID, (char *)CmdBuff+2);
					pWFSID = UD.CfgData.WFSID;
					SetWifiName((unsigned char *)pWFSID);
					WriteCfg();
				}
				else if (CmdBuff[1] == '2')
				{
					strcpy(UD.CfgData.WFPWD, (char *)CmdBuff+2);
					pWFPWD = UD.CfgData.WFPWD;	
					SetWifiCode((unsigned char *)pWFPWD);
					WriteCfg();
				}
				else if (CmdBuff[1] == '3')
				{
					strcpy(UD.CfgData.WFRIP, (char *)CmdBuff+2);
					pWFRIP = UD.CfgData.WFRIP;
					SetRemoteHost((unsigned char *)pWFRIP,(unsigned char *)pWFRPO);
					WriteCfg();
				}
				else if (CmdBuff[1] == '4')
				{
					strcpy(UD.CfgData.WFRPO, (char *)CmdBuff+2);
					pWFRPO = UD.CfgData.WFRPO;
					SetRemoteHost((unsigned char *)pWFRIP,(unsigned char *)pWFRPO);
					WriteCfg();
				}
				sprintf(ReplyBuff,":W#\n");
				break;
			}
		case 'B': //Blue tooth setting  			
			{
				if (CmdBuff[1] == '1')							
				{
					strcpy(UD.CfgData.BTSID, (char *)CmdBuff+2);
					pBTSID = UD.CfgData.BTSID;	
					WriteCfg();
				}
				else if (CmdBuff[1] == '2')
				{
					strcpy(UD.CfgData.BTPWD, (char *)CmdBuff+2);
					pBTPWD = UD.CfgData.BTPWD;	
					WriteCfg();
				}
				sprintf(ReplyBuff,":B#\n");
				break;
			}
		case 'g': //Get config infomation
			{
				ReadCfg(); 
				sprintf(ReplyBuff, ":g%s-%s-%s-%s-%s-%s-%s-%s-%d-%d-%d-%d#\n",
				UD.CfgData.Device,UD.CfgData.Version,UD.CfgData.WFSID,UD.CfgData.WFPWD,UD.CfgData.WFRIP,UD.CfgData.WFRPO,UD.CfgData.BTSID,UD.CfgData.BTPWD,UD.CfgData.IsReverse,UD.CfgData.SubDivision,UD.CfgData.MotorSpeed,UD.CfgData.Position);
				break;
			}
		default://Unknow Command
			{
				sprintf(ReplyBuff, ":U#\n");
				break;
			}
	}
	memset(CmdBuff,0,32);
	return true;
}

int main()
{	
	LED_GPIO_Config();

	SysTick_Init();
	
	USART_Config();

	//HC05_Init();
	BLT_USART_Config();
	
	BASIC_TIM_Init();
	
	EXTI_44E_Config();

	WifiUSART_Config();
	
	ESP8266IO();

	if( DS18B20_Init())	
		//printf("\r\n no ds18b20 exit \r\n");
		bTempAvail=false;
	else
		bTempAvail=true;
	//printf("\r\n ds18b20 exit \r\n");
	
	ReadCfg();
	if((UD.CfgData.IsReverse>1)||(UD.CfgData.SubDivision<1)||(UD.CfgData.MotorSpeed<1))//未初始化过就初始化
	{
		strcpy(UD.CfgData.Device, pEquipment);
		strcpy(UD.CfgData.Version, pVersion);
		strcpy(UD.CfgData.WFSID, pWFSID);
		strcpy(UD.CfgData.WFPWD, pWFPWD);
		strcpy(UD.CfgData.WFRIP, pWFRIP);
		strcpy(UD.CfgData.WFRPO, pWFRPO);
		strcpy(UD.CfgData.BTSID, pBTSID);
		strcpy(UD.CfgData.BTPWD, pBTPWD);
		UD.CfgData.IsReverse=(bIsReverse==true)?1:0;
		UD.CfgData.SubDivision=uSubdivision;
		UD.CfgData.MotorSpeed=uSpeed;
		//UD.CfgData.Position=iStepCount;
		WriteCfg();
	}
	pEquipment = UD.CfgData.Device;
	pVersion=UD.CfgData.Version;
	pWFSID = UD.CfgData.WFSID;
	pWFPWD = UD.CfgData.WFPWD;
	pWFRIP = UD.CfgData.WFRIP;
	pWFRPO = UD.CfgData.WFRPO;
	pBTSID = UD.CfgData.BTSID;
	pBTPWD = UD.CfgData.BTPWD;
	bIsReverse = UD.CfgData.IsReverse==0?false:true;
	uSubdivision = UD.CfgData.SubDivision;
	uSpeed  = UD.CfgData.MotorSpeed;
	//iStepCount = UD.CfgData.Position;
	
	SetMode(uSubdivision);
	SetSpeed(uSpeed);
	
	while(1)
	{	
		//处理串口1数据
		if(UART_RxCmd>0)
		{
			UART_RxCmd--;//命令计数减一
			if(CmdProcess(UART_RxBuffer,&UART_RxPtr_Prv))
				printf("%s", ReplyBuff);
		}	
	
	  //处理串口2数据	
	  if (BLTUART_RxCmd>0)
	  { 
			BLTUART_RxCmd--;//命令计数减一
			if(CmdProcess(BLTUART_RxBuffer,&BLTUART_RxPtr_Prv))
				BLTUsart_SendString(USART2,ReplyBuff);
	  }
		
		//处理串口3数据	
	  if (WIFIUART_RxCmd>0)
	  { 
			WIFIUART_RxCmd--;//命令计数减一
			if(CmdProcess(WIFIUART_RxBuffer,&WIFIUART_RxPtr_Prv))
				WifiUsart_SendString(USART3, ReplyBuff);
	  }
		
		memset(ReplyBuff,0,128);
		
	  switch(uMoveState)
		{
			case 1://Move Out
			{
				iStepDelta = iStepCount - iStartStep;
				if(iStepDelta<iHalfStepDelta)
				{
					Accelerate(iStepDelta);
				}
				else
				{
					iStepDelta = iStopStep - iStepCount;
					Decelerate(iStepDelta);
				}
				break;
			}	
			case 2://Move In
			{
				iStepDelta = iStartStep - iStepCount;
				if(iStepDelta<iHalfStepDelta)
					Accelerate(iStepDelta);
				else 
				{
					iStepDelta = iStepCount - iStopStep;
					Decelerate(iStepDelta);
				}
				break;
			}
			case 3://Slew Out
			{
				iStepDelta = iStepCount - iStartStep;
				if(iStepDelta>=0)
					Accelerate(iStepDelta);
				break;
			}
			case 4://Slew In
			{
				iStepDelta = iStartStep - iStepCount;
				if(iStepDelta>=0)
					Accelerate(iStepDelta);
				break;
			}
			case 5://Stop Slew Out
			{
				iStepDelta = iStopStep - iStepCount;
				if(iStepDelta>=0)
					Decelerate(iStepDelta);
				break;
			}
			case 6://Stop Slew In
			{
				iStepDelta = iStepCount - iStopStep;
				if(iStepDelta>=0)
					Decelerate(iStepDelta);
				break;
			}
			default://Idle
			{
				Halt();
				//uMoveState=0;
				break;
			}
		}
	}
}



/*********************************************END OF FILE**********************/
