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

//extern volatile uint32_t time;
extern bool bIsMoving;	           
extern bool bIncreCount;            
extern int  iStepCount;
	
//接收数组指针
extern unsigned char UART_RxPtr;


extern void TimingDelay_Decrement(void);

u8 flagrun = 0;

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

void DEBUG_USART_IRQHandler(void)
{
	unsigned char data;
    
	if(USART_GetITStatus(DEBUG_USARTx,USART_IT_RXNE)!=RESET)

	{	
		data = USART_ReceiveData(DEBUG_USARTx);
		
	    if(UART_RxPtr < (UART_RX_BUFFER_SIZE - 1))
        {
                UART_RxBuffer[UART_RxPtr] = data;
                UART_RxBuffer[UART_RxPtr + 1]=0x00;
                UART_RxPtr++;
        }
		else
        {
                UART_RxBuffer[UART_RxPtr - 1] = data;
                
        }
		
        //如果为回车键，则开始处理串口数据
        if(data == 35)
        {
            flagrun = 1;
        }
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

void BLT_USART_IRQHandler(void)
{
	bsp_USART_Process();

}

void MIN_IRQHandler(void)
{
  //确保是否产生了EXTI Line中断
	if(EXTI_GetITStatus(MIN_INT_EXTI_LINE) != RESET) 
	{
		iStepCount = -65535;
		printf("it is min %d!\n", iStepCount);
		ControlMotor(DISABLE);
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
		ControlMotor(DISABLE);
   		//清除中断标志位
		EXTI_ClearITPendingBit(MAX_INT_EXTI_LINE);     
	}  
}
// 串口中断服务函数
bool bRunMotor = false;
void DEBUG_USART3_IRQHandler(void) 
{
  	
	unsigned char data;
  	
	if(USART_GetITStatus(DEBUG_USART3x,USART_IT_RXNE)!=RESET)
	{		
    
			data = USART_ReceiveData(DEBUG_USART3x);
		
    	if(WIFIUART_RxPtr < (WIFIUART_RX_BUFFER_SIZE - 1))
        {
                WIFIUART_RxBuffer[WIFIUART_RxPtr] = data;
                WIFIUART_RxBuffer[WIFIUART_RxPtr + 1]=0x00;
                WIFIUART_RxPtr++;
        }
			else
        {
                WIFIUART_RxBuffer[WIFIUART_RxPtr - 1] = data;
                
        }

		if (data == 35)
		{
			bRunMotor = true;
		}
	}	 
}


/******************* (C) COPYRIGHT 2011 STMicroelectronics *****END OF FILE****/
