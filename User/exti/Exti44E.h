#ifndef __Exti44E_H
#define __Exti44E_H

#include "stm32f10x.h"

#define MIN_INT_GPIO_PORT         GPIOB
#define MIN_INT_GPIO_CLK          (RCC_APB2Periph_GPIOB|RCC_APB2Periph_AFIO)
#define MIN_INT_GPIO_PIN          GPIO_Pin_0
#define MIN_INT_EXTI_PORTSOURCE   GPIO_PortSourceGPIOB
#define MIN_INT_EXTI_PINSOURCE    GPIO_PinSource0
#define MIN_INT_EXTI_LINE         EXTI_Line0
#define MIN_INT_EXTI_IRQ          EXTI0_IRQn

#define MIN_IRQHandler            EXTI0_IRQHandler


#define MAX_INT_GPIO_PORT         GPIOB
#define MAX_INT_GPIO_CLK          (RCC_APB2Periph_GPIOB|RCC_APB2Periph_AFIO)
#define MAX_INT_GPIO_PIN          GPIO_Pin_1
#define MAX_INT_EXTI_PORTSOURCE   GPIO_PortSourceGPIOB
#define MAX_INT_EXTI_PINSOURCE    GPIO_PinSource1
#define MAX_INT_EXTI_LINE         EXTI_Line1
#define MAX_INT_EXTI_IRQ          EXTI1_IRQn

#define MAX_IRQHandler            EXTI1_IRQHandler


void EXTI_44E_Config(void);

#endif

