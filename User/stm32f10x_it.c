/**
  ******************************************************************************
  * @file    Project/STM32F10x_StdPeriph_Template/stm32f10x_it.c 
  * @author  MCD Application Team
  * @version V3.5.0
  * @date    08-April-2011
  * @brief   Main Interrupt Service Routines.
  *          This file provides template for all exceptions handler and 
  *          peripherals interrupt service routine.
  ******************************************************************************
  * @attention
  *
  * THE PRESENT FIRMWARE WHICH IS FOR GUIDANCE ONLY AIMS AT PROVIDING CUSTOMERS
  * WITH CODING INFORMATION REGARDING THEIR PRODUCTS IN ORDER FOR THEM TO SAVE
  * TIME. AS A RESULT, STMICROELECTRONICS SHALL NOT BE HELD LIABLE FOR ANY
  * DIRECT, INDIRECT OR CONSEQUENTI
  
  AL DAMAGES WITH RESPECT TO ANY CLAIMS ARISING
  * FROM THE CONTENT OF SUCH FIRMWARE AND/OR THE USE MADE BY CUSTOMERS OF THE
  * CODING INFORMATION CONTAINED HEREIN IN CONNECTION WITH THEIR PRODUCTS.
  *
  * <h2><center>&copy; COPYRIGHT 2011 STMicroelectronics</center></h2>
  ******************************************************************************
  */

/* Includes ------------------------------------------------------------------*/
#include "stm32f10x_it.h"
#include "bsp_usart_blt.h"
#include "bsp_usart.h"
#include "bsp_TiMbase.h"
#include "stdbool.h"
#include "bsp_led.h"
#include "Exti44E.h"
#include "WifiUsart.h"

extern bool bIsMoving;	           
extern bool bIncreCount;            
extern int  iStepCount;
unsigned char UART_RxPtr_Start=0;//命令开始位置即:位置
unsigned char BLTUART_RxPtr_Start=0;//命令开始位置即:位置
unsigned char WIFIUART_RxPtr_Start=0;//命令开始位置即:位置
extern unsigned char UART_RxPtr_Prv;
extern unsigned char BLTUART_RxPtr_Prv;
extern unsigned char WIFIUART_RxPtr_Prv;
extern void Halt(void);
extern void TimingDelay_Decrement(void);

/** @addtogroup STM32F10x_StdPeriph_Template
  * @{
  */

/* Private typedef -----------------------------------------------------------*/
/* Private define ------------------------------------------------------------*/
/* Private macro -------------------------------------------------------------*/
/* Private variables ---------------------------------------------------------*/
/* Private function prototypes -----------------------------------------------*/
/* Private functions ---------------------------------------------------------*/

/******************************************************************************/
/*            Cortex-M3 Processor Exceptions Handlers                         */
/******************************************************************************/

/**
  * @brief  This function handles NMI exception.
  * @param  None
  * @retval None
  */
void NMI_Handler(void)
{
}

/**
  * @brief  This function handles Hard Fault exception.
  * @param  None
  * @retval None
  */
void HardFault_Handler(void)
{
  /* Go to infinite loop when Hard Fault exception occurs */
  while (1)
  {
  }
}

/**
  * @brief  This function handles Memory Manage exception.
  * @param  None
  * @retval None
  */
void MemManage_Handler(void)
{
  /* Go to infinite loop when Memory Manage exception occurs */
  while (1)
  {
  }
}

/**
  * @brief  This function handles Bus Fault exception.
  * @param  None
  * @retval None
  */
void BusFault_Handler(void)
{
  /* Go to infinite loop when Bus Fault exception occurs */
  while (1)
  {
  }
}

/**
  * @brief  This function handles Usage Fault exception.
  * @param  None
  * @retval None
  */
void UsageFault_Handler(void)
{
  /* Go to infinite loop when Usage Fault exception occurs */
  while (1)
  {
  }
}

/**
  * @brief  This function handles SVCall exception.
  * @param  None
  * @retval None
  */
void SVC_Handler(void)
{
}

/**
  * @brief  This function handles Debug Monitor exception.
  * @param  None
  * @retval None
  */
void DebugMon_Handler(void)
{
}

/**
  * @brief  This function handles PendSVC exception.
  * @param  None
  * @retval None
  */
void PendSV_Handler(void)
{
}


void  BASIC_TIM_IRQHandler (void)
{
	if ( TIM_GetITStatus( BASIC_TIM, TIM_IT_Update) != RESET ) 
	{	

		
		if ((bIsMoving) && (bIncreCount) )
		{
			iStepCount++;	
		}								
		else if ((bIsMoving) && (!bIncreCount))
		{
			iStepCount--;
		}

	
			
		TIM_ClearITPendingBit(BASIC_TIM , TIM_FLAG_Update); 

        
	}		 	
}

void SysTick_Handler(void)
{
	TimingDelay_Decrement();	
}

u8 Delt(u8 Cur,u8 Prv)
{
	if(Cur>=Prv)
		return Cur-Prv;
	else
		return Cur-Prv+257;//保证大于等于256
}

// 串口1中断服务函数
void DEBUG_USART_IRQHandler(void)
{
	if(USART_GetITStatus(DEBUG_USARTx,USART_IT_RXNE)!=RESET)
	{
		//循环接收
		UART_RxBuffer[UART_RxPtr] = USART_ReceiveData(DEBUG_USARTx);
		if(UART_RxBuffer[UART_RxPtr] == 58)
			UART_RxPtr_Start=UART_RxPtr;
		if(Delt(UART_RxPtr,UART_RxPtr_Prv)>=UART_RX_BUFFER_SIZE)//超过缓存就丢弃
		{
			UART_RxPtr_Prv=UART_RxPtr_Start;
			UART_RxCmd=0;
		}
		if(UART_RxBuffer[UART_RxPtr] == 35)
			UART_RxCmd++;
		if(UART_RxPtr < (UART_RX_BUFFER_SIZE - 1))//256-1
			UART_RxPtr++;
		else
			UART_RxPtr=0;
	}	
}
// 串口2中断服务函数
void BLT_USART_IRQHandler(void)
{
  if(USART_GetITStatus(BLT_USARTx, USART_IT_RXNE) != RESET)
	{
		//循环接收
		BLTUART_RxBuffer[BLTUART_RxPtr] = USART_ReceiveData(BLT_USARTx);
		if(BLTUART_RxBuffer[BLTUART_RxPtr] == 58)
			BLTUART_RxPtr_Start=BLTUART_RxPtr;
		if(Delt(BLTUART_RxPtr,BLTUART_RxPtr_Prv)>=BLTUART_RX_BUFFER_SIZE)//超过缓存就丢弃
		{
			BLTUART_RxPtr_Prv=BLTUART_RxPtr_Start;
			BLTUART_RxCmd=0;
		}
		if(BLTUART_RxBuffer[BLTUART_RxPtr] == 35)
			BLTUART_RxCmd++;
		if(BLTUART_RxPtr < (BLTUART_RX_BUFFER_SIZE- 1))//256-1
			BLTUART_RxPtr++;
		else
			BLTUART_RxPtr=0;
	}	
}
// 串口3中断服务函数
void DEBUG_USART3_IRQHandler(void) 
{
  if(USART_GetITStatus(DEBUG_USART3x,USART_IT_RXNE)!=RESET)
	{
		//循环接收
		WIFIUART_RxBuffer[WIFIUART_RxPtr] = USART_ReceiveData(DEBUG_USART3x);
		if(WIFIUART_RxBuffer[WIFIUART_RxPtr] == 58)
			WIFIUART_RxPtr_Start=WIFIUART_RxPtr;
		if(Delt(WIFIUART_RxPtr,WIFIUART_RxPtr_Prv)>=WIFIUART_RX_BUFFER_SIZE)//超过缓存就丢弃
		{
			WIFIUART_RxPtr_Prv=WIFIUART_RxPtr_Start;
			WIFIUART_RxCmd=0;
		}
		if(WIFIUART_RxBuffer[WIFIUART_RxPtr] == 35)
			WIFIUART_RxCmd++;
		if(WIFIUART_RxPtr < (WIFIUART_RX_BUFFER_SIZE - 1))//256-1
			WIFIUART_RxPtr++;
		else
			WIFIUART_RxPtr=0;
	}		
}

/******************************************************************************/
/*                 STM32F10x Peripherals Interrupt Handlers                   */
/*  Add here the Interrupt Handler for the used peripheral(s) (PPP), for the  */
/*  available peripheral interrupt handler's name please refer to the startup */
/*  file (startup_stm32f10x_xx.s).                                            */
/******************************************************************************/

/**
  * @brief  This function handles PPP interrupt request.
  * @param  None
  * @retval None
  */
/*void PPP_IRQHandler(void)
{
}*/

/**
  * @}
  */ 

void MIN_IRQHandler(void)
{
  //确保是否产生了EXTI Line中断
	if(EXTI_GetITStatus(MIN_INT_EXTI_LINE) != RESET) 
	{
		iStepCount = 0;
		printf("it is min %d!\n", iStepCount);
		//ControlMotor(DISABLE);
		Halt();
    	//清除中断标志位
		EXTI_ClearITPendingBit(MIN_INT_EXTI_LINE);     
	}  
}

void MAX_IRQHandler(void)
{
  //确保是否产生了EXTI Line中断
	if(EXTI_GetITStatus(MAX_INT_EXTI_LINE) != RESET) 
	{
		iStepCount = 65535;
		printf("it is max %d!\n", iStepCount);
		//ControlMotor(DISABLE);
		Halt();
   		//清除中断标志位
		EXTI_ClearITPendingBit(MAX_INT_EXTI_LINE);     
	}  
}


/******************* (C) COPYRIGHT 2011 STMicroelectronics *****END OF FILE****/
