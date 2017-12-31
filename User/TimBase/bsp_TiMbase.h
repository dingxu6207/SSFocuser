#ifndef __BSP_TIMEBASE_H
#define __BSP_TIMEBASE_H

#include "stm32f10x.h"

#define            BASIC_TIM                   TIM1
#define            BASIC_TIM_APBxClock_FUN     RCC_APB2PeriphClockCmd
#define            BASIC_TIM_CLK               RCC_APB2Periph_TIM1
#define            BASIC_TIM_Period            (10000-1)    
#define            BASIC_TIM_Prescaler         (7200-1)
#define            BASIC_TIM_IRQ               TIM1_UP_IRQn
#define            BASIC_TIM_IRQHandler        TIM1_UP_IRQHandler

// TIM1 输出比较通道
#define ADVANCE_TIM_CH1_GPIO_CLK 		RCC_APB2Periph_GPIOA
#define ADVANCE_TIM_CH1_PORT 			GPIOA
#define ADVANCE_TIM_CH1_PIN 			GPIO_Pin_8


void BASIC_TIM_Init(void);
void ControlMotor(FunctionalState NewState);
void SetSpeedMoter(u16 SpeedMoter);


#endif	/* __BSP_TIMEBASE_H */

