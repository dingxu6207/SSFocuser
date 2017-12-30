#ifndef __HC05_H
#define	__HC05_H
#include "stm32f10x.h"
#include "bsp_SysTick.h"



void HC05_Init(void);
int get_line(char* line, char* stream ,int max_size); 
void HC05_SendString(char* str);


#endif /* _HC05_H */

