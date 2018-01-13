#ifndef __WIFIUSART_H
#define	__WIFIUSART_H

#include "stm32f10x.h"
#include <stdio.h>

//串口接收缓冲数组大小
#define WIFIUART_RX_BUFFER_SIZE 1024
extern unsigned char WIFIUART_RxBuffer[WIFIUART_RX_BUFFER_SIZE];
extern unsigned char WIFIUART_RxPtr;

// 串口3-USART3
#define  DEBUG_USART3x                   USART3
#define  DEBUG_USART3_CLK                RCC_APB1Periph_USART3
#define  DEBUG_USART3_APBxClkCmd         RCC_APB1PeriphClockCmd
#define  DEBUG_USART3_BAUDRATE           115200

// USART GPIO 引脚宏定义
#define  DEBUG_USART3_GPIO_CLK           (RCC_APB2Periph_GPIOB)
#define  DEBUG_USART3_GPIO_APBxClkCmd    RCC_APB2PeriphClockCmd
    
#define  DEBUG_USART3_TX_GPIO_PORT         GPIOB   
#define  DEBUG_USART3_TX_GPIO_PIN          GPIO_Pin_10
#define  DEBUG_USART3_RX_GPIO_PORT       GPIOB
#define  DEBUG_USART3_RX_GPIO_PIN        GPIO_Pin_11

#define  DEBUG_USART3_IRQ                USART3_IRQn
#define  DEBUG_USART3_IRQHandler         USART3_IRQHandler


void WifiUSART_Config(void);
void WifiUsart_SendByte( USART_TypeDef * pUSARTx, uint8_t ch);
void WifiUsart_SendString( USART_TypeDef * pUSARTx, char *str);
void Wifiuart_FlushRxBuffer(void);




#endif
